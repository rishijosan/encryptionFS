//*****************************************************************************
// CSE-506 Operating Systems - Spring 2013
// 
// Home Work Assignment 3
//
// Author   - Rishi Josan 
// SBU ID # - 108996773 
// Version  - 1.0
// Date     - 7 April 2013
//
//*****************************************************************************

#include "wrapfs.h"
#include <linux/crypto.h>
#include <crypto/hash.h>
#include <linux/scatterlist.h>

/* This function reads a file in chunks of size PAGE_SIZE and 
 * incrementally computes its MD5 value */
/* Adapted from HW1 solution */

int wrapfs_compute_md5(u8 * integ_val, char *key, int keySize)
{

	int ret = 0, err = 0;
	mm_segment_t oldfs;
	struct crypto_hash *crypt_hash;
	struct hash_desc desc;
	struct scatterlist sg;

	/* Setting crypto hash to MD5 for computation of MD5 */

	crypt_hash = crypto_alloc_hash("md5", 0, CRYPTO_ALG_ASYNC);

	if (IS_ERR(crypt_hash)) {
		printk(KERN_ERR "Unable to allocate struct crypto_hash");
		err = -EPERM;
		goto out_md5;
	}

	desc.tfm = crypt_hash;
	desc.flags = 0;

	/* Initialization for crypto API  */
	if (crypto_hash_init(&desc) < 0) {
		printk(KERN_ERR "Crypto_hash_init() failed\n");
		crypto_free_hash(crypt_hash);
		err = -EPERM;
		goto out_md5;
	}

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	/* Passing read buffer to crypto API */
	sg_init_one(&sg, key, keySize);

	/* Updating the MD5 Hash value */
	ret = crypto_hash_update(&desc, &sg, keySize);
	if (ret < 0) {
		printk(KERN_ERR "Crypto_hash_update() failed for\n");
		crypto_free_hash(crypt_hash);
		err = -EPERM;
		goto out_md5;
	}

	set_fs(oldfs);

	ret = crypto_hash_final(&desc, integ_val);
	if (ret < 0) {
		printk(KERN_ERR "Crypto_hash_final() failed\n");
		crypto_free_hash(crypt_hash);
		err = -EPERM;
		goto out_md5;
	}

out_md5:

	if (err < 0)
		return err;
	else
		return 0;

}

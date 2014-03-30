/**
 * Encryption functions for wrapfs
 * Based on fs/ecryptfs/crypto.c
 *
 */

#include <linux/fs.h>
#include <linux/mount.h>
#include <linux/pagemap.h>
#include <linux/random.h>
#include <linux/compiler.h>
#include <linux/key.h>
#include <linux/namei.h>
#include <linux/crypto.h>
#include <linux/file.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include "wrapfs.h"

/* Reference: http://wbsun.blogspot.com/2011/01/use-kernel-crypto-lib.html */
int wrapfs_encrypt_page_new(struct page *page, struct page *dst_page,
			    u8 * encKey)
{
	struct crypto_blkcipher *tfm;
	struct blkcipher_desc desc;
	int i;
	struct scatterlist src;
	struct scatterlist dst;

	/* char *buf; */
	/* char **ins, **outs; */
	unsigned int ret = 0;

	/*     u8 key[] = {0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 
	   0x08, 0x0A, 0x0B, 0x0C, 0x0D, 0x0F, 0x10, 0x11, 0x12};

	 */
	WDBG("Enc Key: \n");
	for (i = 0; i < MD5_SIGNATURE_SIZE; i++) {
		WDBG("%02x", encKey[i] & 0xff);
	}
	WDBG("\n");
	WDBG("Size Of encKey = %d\n", sizeof(encKey));

	/*
	   buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	   if(!buf){
	   printk(KERN_ERR "Could not kmalloc buf\n");

	   return -ENOMEM;

	   }
	   buf = "ABCDEFG!!!";
	 */

	/* printk(KERN_DEBUG "Before alloc blkcipher\n"); */
	tfm = crypto_alloc_blkcipher("ctr(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		printk(KERN_ERR "failed to load transform");
		goto out;
	}
	desc.tfm = tfm;
	desc.flags = 0;
	WDBG("Before setkey\n");
	ret = crypto_blkcipher_setkey(tfm, encKey, MD5_SIGNATURE_SIZE);
	if (ret) {
		printk(KERN_ERR "setkey() failed flags=%x\n",
		       crypto_blkcipher_get_flags(tfm));
		goto out;
	}
	WDBG("Before init table\n");
	sg_init_table(&src, 1);
	sg_init_table(&dst, 1);
	sg_set_page(&src, page, PAGE_SIZE, 0);
	sg_set_page(&dst, dst_page, PAGE_SIZE, 0);

	/* printk(KERN_DEBUG "Before blkcipher encrypt\n"); */
	ret = crypto_blkcipher_encrypt(&desc, &dst, &src, PAGE_SIZE);
	if (ret < 0) {
		printk(KERN_ERR "Encryption Failed, Ret: %d\n", ret);
		goto out;
	}
out:
	return ret;
}

int wrapfs_decrypt_page_new(struct page *page, struct page *dst_page, u8 * key)
{
	struct crypto_blkcipher *tfm;
	struct blkcipher_desc desc;
	struct scatterlist src;
	struct scatterlist dst;

	/* char *buf; */
	/* char **ins, **outs; */
	unsigned int ret = 0;

	/*     
	   u8 key[] = {0x00, 0x01, 0x02, 0x03, 0x05, 0x06, 0x07, 
	   0x08, 0x0A, 0x0B, 0x0C, 0x0D, 0x0F, 0x10, 0x11, 0x12};
	 */

	/*
	   buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
	   if(!buf){
	   WDBG("Could not kmalloc buf\n");

	   return -ENOMEM;

	   }
	   buf = "ABCDEFG!!!";
	 */
	WDBG("Before alloc blkcipher\n");
	tfm = crypto_alloc_blkcipher("ctr(aes)", 0, 0);
	if (IS_ERR(tfm)) {
		printk(KERN_ERR "failed to load transform");
		goto out;
	}
	desc.tfm = tfm;
	desc.flags = 0;
	WDBG("Before setkey\n");
	ret = crypto_blkcipher_setkey(tfm, key, MD5_SIGNATURE_SIZE);
	if (ret) {
		printk(KERN_ERR "setkey() failed flags=%x\n",
		       crypto_blkcipher_get_flags(tfm));
		goto out;
	}
	WDBG("Before init table\n");
	sg_init_table(&src, 1);
	sg_init_table(&dst, 1);
	sg_set_page(&src, page, PAGE_SIZE, 0);
	sg_set_page(&dst, dst_page, PAGE_SIZE, 0);
	WDBG("Before blkcipher decrypt\n");
	ret = crypto_blkcipher_decrypt(&desc, &src, &dst, PAGE_SIZE);
	if (ret < 0) {
		printk(KERN_ERR "Encryption Failed, Ret: %d\n", ret);
		goto out;
	}
out:
	return ret;
}

/*
 * Copyright (c) 1998-2011 Erez Zadok
 * Copyright (c) 2009	   Shrikar Archak
 * Copyright (c) 2003-2011 Stony Brook University
 * Copyright (c) 2003-2011 The Research Foundation of SUNY
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "wrapfs.h"
#include <linux/page-flags.h>
#include <linux/pagemap.h>
#include <linux/writeback.h>


//readpage adapted from unionfs
/* Readpage expects a locked page, and must unlock it */
static int wrapfs_readpage(struct file *file, struct page *page)
{
	int err;
	struct file *lower_file;
	struct inode *inode, *lower_inode;
	mm_segment_t old_fs;
	char *page_data = NULL;
	mode_t orig_mode;
	loff_t offset;
	
	#ifdef WRAPFS_CRYPTO
	struct super_block *sb;	
	u8 * encKey;
	struct page *dst_page = NULL;
	char *dst_virt = NULL;
	#endif
	
	WDBG("In Readpage!!!\n");
	
	//New Stuff
    offset = ((((loff_t)page->index) << PAGE_CACHE_SHIFT));
	
	

	//Vars for decryption
	#ifdef WRAPFS_CRYPTO
	
	
	sb = file->f_dentry->d_sb;
	encKey = wrapfs_get_enc_key(sb);

	
	if (encKey != NULL){

	
	// Allocate page - decryption	
	dst_page = alloc_page(GFP_USER);
	 if (!dst_page) {
                 err = -ENOMEM;
                 printk(KERN_ERR "Error allocating memory for "
                                 "encrypted extent\n");
                 goto out;
         }
	
	dst_virt = kmap(dst_page);
	//printk("Dest_virt: %p\n" , dst_virt);
	}
	else{
	printk("Encryption Key Not Set!\n");
	return -EPERM;
	}
	#endif
	

	/*
	wrapfs_read_lock(file->f_path.dentry->d_sb, WRAPFS_SMUTEX_PARENT);
	err = wrapfs_file_revalidate(file, false);
	if (unlikely(err))
		goto out;
	wrapfs_check_file(file);
	*/

	
	if (!WRAPFS_F(file)) {
		err = -ENOENT;
		goto out;
	}
	

	
	

	lower_file = wrapfs_lower_file(file);
	/* FIXME: is this assertion right here? */
	BUG_ON(lower_file == NULL);

	inode = file->f_path.dentry->d_inode;
	
	//New Code Why?
	lower_inode = wrapfs_lower_inode(inode);

	
	//See if char * needed
	page_data = (char *) kmap(page);
	/*
	 * Use vfs_read because some lower file systems don't have a
	 * readpage method, and some file systems (esp. distributed ones)
	 * don't like their pages to be accessed directly.  Using vfs_read
	 * may be a little slower, but a lot safer, as the VFS does a lot of
	 * the necessary magic for us.
	 */
	lower_file->f_pos = page_offset(page);
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	/* get_ds())
	*/
	 /*
	 * generic_file_splice_write may call us on a file not opened for
	 * reading, so temporarily allow reading.
	 */
	orig_mode = lower_file->f_mode;
	lower_file->f_mode |= FMODE_READ;
	
	
//Changed the offset here : see backup 8 for previous
	#ifdef WRAPFS_CRYPTO
	if (encKey != NULL)
	err = vfs_read(lower_file, dst_virt, PAGE_CACHE_SIZE, &lower_file->f_pos);
	else
	err = vfs_read(lower_file, page_data, PAGE_CACHE_SIZE,&lower_file->f_pos);
	#else
	err = vfs_read(lower_file, page_data, PAGE_CACHE_SIZE,&lower_file->f_pos);
	#endif
	//Decrypt After Read	
		   
	//printk("Err: %d" , err);	

	if (err > 0)
	err = 0;	
			   
	lower_file->f_mode = orig_mode;
	set_fs(old_fs);
	
/* 	if (err >= 0 && err < PAGE_CACHE_SIZE)
		memset(page_data + err, 0, PAGE_CACHE_SIZE - err); */
		
/* 		if (err >= 0 && err < PAGE_CACHE_SIZE)
		memset(dst_virt + err, 0, PAGE_CACHE_SIZE - err);  */
		
	#ifdef WRAPFS_CRYPTO
	if (encKey != NULL)	{
	err = wrapfs_decrypt_page_new(page, dst_page, encKey);
	//printk("Decrypted Data: %s" , page_data );	
	}
	#endif
		
	kunmap(page);

	if (err < 0){
	//printk("Error<0");
		goto out;
		}
	


	/* if vfs_read succeeded above, sync up our times */
	//wrapfs_copy_attr_times(inode);
	
	

	flush_dcache_page(page);
	fsstack_copy_attr_atime(inode, lower_inode);

	/*
	 * we have to unlock our page, b/c we _might_ have gotten a locked
	 * page.  but we no longer have to wakeup on our page here, b/c
	 * UnlockPage does it
	 */
out:
	if (err == 0)
		SetPageUptodate(page);
	else
		ClearPageUptodate(page);

	unlock_page(page);
	#ifdef WRAPFS_CRYPTO
	if (encKey != NULL)
	kunmap(dst_page);
	#endif
	//wrapfs_check_file(file);
	//wrapfs_read_unlock(file->f_path.dentry->d_sb);

	return err;
}



//writepage adapted from unionfs
static int wrapfs_writepage(struct page *page, struct writeback_control *wbc)
{
	int err = -EIO;
	struct inode *inode;
	struct inode *lower_inode;
	struct page *lower_page;
	struct address_space *lower_mapping; /* lower inode mapping */
	gfp_t mask;
	
	WDBG("In WritePage!\n");
	

	BUG_ON(!PageUptodate(page));
	inode = page->mapping->host;
	/* if no lower inode, nothing to do */
	if (!inode || !WRAPFS_I(inode) || WRAPFS_I(inode)->lower_inode) {
		err = 0;
		goto out;
	}
	lower_inode = wrapfs_lower_inode(inode);
	lower_mapping = lower_inode->i_mapping;

	mask = mapping_gfp_mask(lower_mapping) & ~(__GFP_FS);
	lower_page = find_or_create_page(lower_mapping, page->index, mask);
	if (!lower_page) {
		err = 0;
		set_page_dirty(page);
		goto out;
	}


	copy_highpage(lower_page, page);
	flush_dcache_page(lower_page);
	SetPageUptodate(lower_page);
	set_page_dirty(lower_page);

	if (wbc->for_reclaim) {
		unlock_page(lower_page);
		goto out_release;
	}

	BUG_ON(!lower_mapping->a_ops->writepage);
	wait_on_page_writeback(lower_page); /* prevent multiple writers */
	clear_page_dirty_for_io(lower_page); /* emulate VFS behavior */
	err = lower_mapping->a_ops->writepage(lower_page, wbc);
	if (err < 0)
		goto out_release;


	if (err == AOP_WRITEPAGE_ACTIVATE) {
		err = 0;
		unlock_page(lower_page);
	}

	//wrapfs_copy_attr_times(inode);
	//printk("Copying Attr Time\n");
	fsstack_copy_inode_size(inode, lower_inode);
	fsstack_copy_attr_times(inode, lower_inode);

out_release:
	/* b/c find_or_create_page increased refcnt */
	page_cache_release(lower_page);
out:

	unlock_page(page);
	return err;
}


	
	

static int wrapfs_write_begin(struct file *file,
			struct address_space *mapping,
			loff_t pos, unsigned len, unsigned flags,
			struct page **pagep, void **fsdata)
{

	struct page *page;
	pgoff_t index = pos >> PAGE_CACHE_SHIFT;
	loff_t prev_page_end_size;
	int rc = 0;
	
	WDBG("In write_begin!\n");
	
	

	
#ifdef WRAPFS_CRYPTO
	if (wrapfs_get_enc_key(file->f_dentry->d_sb) == NULL){
	printk("Encryption Key Not Set!\n");
	//wrapfs_set_lower_file(file, NULL);
	return -EPERM;
	}
#endif
	
	page = grab_cache_page_write_begin(mapping, index, flags);
	if (!page)
		return -ENOMEM;
	*pagep = page;
	
	prev_page_end_size = ((loff_t)index << PAGE_CACHE_SHIFT);
	
	if (!PageUptodate(page) && (len != PAGE_CACHE_SIZE)) {
	
	unsigned from = pos & (PAGE_CACHE_SIZE - 1);
	zero_user_segments(page, 0, from , from + len , PAGE_CACHE_SIZE);
	
	}
	
	//Added NOW
	SetPageUptodate(page);
	return rc;

}

static int fill_zeros_to_end_of_page(struct page *page, unsigned int to)
{
	struct inode *inode = page->mapping->host;
	int end_byte_in_page;

	if ((i_size_read(inode) / PAGE_CACHE_SIZE) != page->index)
		goto out;
	end_byte_in_page = i_size_read(inode) % PAGE_CACHE_SIZE;
	if (to > end_byte_in_page)
		end_byte_in_page = to;
	zero_user_segment(page, end_byte_in_page, PAGE_CACHE_SIZE);
out:
	return 0;
}

static int wrapfs_write_end(struct file *file,
			struct address_space *mapping,
			loff_t pos, unsigned len, unsigned copied,
			struct page *page, void *fsdata)
{
	//pgoff_t index = pos >> PAGE_CACHE_SHIFT;
	unsigned from = pos & (PAGE_CACHE_SIZE - 1);
	unsigned to = from + copied;
	struct inode *wrapfs_inode = mapping->host;
	int need_unlock_page = 1;
	struct file *lower_file;
	char *virt = NULL;
	loff_t offset;
	mm_segment_t fs_save;
	ssize_t rc=0;	
	//int flags;
	//int flag_ch = 0;
	
	#ifdef WRAPFS_CRYPTO
	struct super_block *sb;	
	u8 * encKey;
	int ret;
	struct page *dst_page = NULL;
	char  *dst_virt = NULL;
	#endif
	
	WDBG("In write_end!\n");


	lower_file = wrapfs_lower_file(file);
	if (!lower_file)
        return -EIO;
	
	#ifdef WRAPFS_CRYPTO
	
	sb = file->f_dentry->d_sb;
	encKey = wrapfs_get_enc_key(sb);
	//int doEnc = 0;

	
	//If encKey = NULL, no encryption
	if (encKey != NULL){

	dst_page = alloc_page(GFP_USER);
	 if (!dst_page) {
                 ret = -ENOMEM;
                 printk(KERN_ERR "Error allocating memory for "
                                 "encrypted extent\n");
                 goto out;
         }
	
	dst_virt = kmap(dst_page);
	//printk("Dest_virt: %p\n" , dst_virt);
	}
    #endif
	
	
	offset = (((loff_t)page->index) << PAGE_CACHE_SHIFT) ;
    virt = kmap(page);
	//Check for NULL?

	fs_save = get_fs();
	set_fs(get_ds());
	
	//flags = file->f_flags;
	
/* 	if ((flags & O_APPEND) == O_APPEND){
	printk("Changing Append Flags\n");
	flags = (flags & ~O_APPEND);
	file->f_flags = flags;
	flag_ch = 1;
	} */
	
	#ifdef WRAPFS_CRYPTO
	if (encKey != NULL){
	// Do Encryption Here
	ret = wrapfs_encrypt_page_new(page, dst_page, encKey);
	//printk("Encrypted text: %s \n" , dst_virt);
	//printk("After encrypt page \n");
	rc = vfs_write(lower_file, dst_virt, copied, &offset);
	}
	else
	rc = vfs_write(lower_file, virt + from, copied, &offset);
	#else
	rc = vfs_write(lower_file, virt + from, copied, &offset);
	#endif
   
/* 	if (flag_ch == 1){
	printk("Reverting Append Flags\n");
	flags = (flags & O_APPEND);
	file->f_flags = flags;
	}	 */
   
    set_fs(fs_save);
    mark_inode_dirty_sync(wrapfs_inode);
	
	if (rc > 0)
    rc = 0;

    kunmap(page);
	if (!rc) {
	
		rc = copied;
		fsstack_copy_inode_size(wrapfs_inode,lower_file->f_path.dentry->d_inode);
	}
		//goto out;
		

//New Stuff
 	rc = fill_zeros_to_end_of_page(page, to);
	if (rc) {
		goto out;
	}
	set_page_dirty(page);
	unlock_page(page);
	need_unlock_page = 0;
	 
	//End of new Stuff

	if (pos + copied > i_size_read(wrapfs_inode)) {
		i_size_write(wrapfs_inode, pos + copied);
		balance_dirty_pages_ratelimited(mapping); // This is new
	}
	rc = copied; // Check

out:
	if (need_unlock_page)
		unlock_page(page);

	page_cache_release(page);
#ifdef WRAPFS_CRYPTO
	if (encKey != NULL)
	kunmap(dst_page);
#endif
	return rc;
}			
	

  

static int wrapfs_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{

	
	int err;
	struct file *file, *lower_file;
	const struct vm_operations_struct *lower_vm_ops;
	struct vm_area_struct lower_vma;

	memcpy(&lower_vma, vma, sizeof(struct vm_area_struct));
	file = lower_vma.vm_file;
	lower_vm_ops = WRAPFS_F(file)->lower_vm_ops;
	BUG_ON(!lower_vm_ops);

	//printk("In wrapfs_fault\n");
	dump_stack();
	
	lower_file = wrapfs_lower_file(file);
	/*
	 * XXX: vm_ops->fault may be called in parallel.  Because we have to
	 * resort to temporarily changing the vma->vm_file to point to the
	 * lower file, a concurrent invocation of wrapfs_fault could see a
	 * different value.  In this workaround, we keep a different copy of
	 * the vma structure in our stack, so we never expose a different
	 * value of the vma->vm_file called to us, even temporarily.  A
	 * better fix would be to change the calling semantics of ->fault to
	 * take an explicit file pointer.
	 */
	lower_vma.vm_file = lower_file;
	err = lower_vm_ops->fault(&lower_vma, vmf);
	return err;
}




//DELETE THIS
static int wrapfs_fault_null(struct vm_area_struct *vma, struct vm_fault *vmf)
{

	return 0;
}


/*
 * XXX: the default address_space_ops for wrapfs is empty.  We cannot set
 * our inode->i_mapping->a_ops to NULL because too many code paths expect
 * the a_ops vector to be non-NULL.
 */
const struct address_space_operations wrapfs_aops = {
	 .writepage = wrapfs_writepage,
         .readpage = wrapfs_readpage,
         .write_begin = wrapfs_write_begin,
         .write_end = wrapfs_write_end,
		 
};


const struct address_space_operations wrapfs_aops_null = {

		 
};

const struct vm_operations_struct wrapfs_vm_ops = {
	.fault		= wrapfs_fault,
	//.fault		= wrapfs_fault_null,
};

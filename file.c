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
#include "wrapfs_ioctl.h"

static ssize_t wrapfs_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	int err;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;
	
	//printk(KERN_ERR "In Read!\n");
	//dump_stack();

	lower_file = wrapfs_lower_file(file);
	err = vfs_read(lower_file, buf, count, ppos);
	/* update our inode atime upon a successful lower read */
	if (err >= 0)
		fsstack_copy_attr_atime(dentry->d_inode,
					lower_file->f_path.dentry->d_inode);

	return err;
}

static ssize_t wrapfs_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *ppos)
{
	int err = 0;
	struct file *lower_file;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = wrapfs_lower_file(file);
	err = vfs_write(lower_file, buf, count, ppos);
	/* update our inode times+sizes upon a successful lower write */
	if (err >= 0) {
		fsstack_copy_inode_size(dentry->d_inode,
					lower_file->f_path.dentry->d_inode);
		fsstack_copy_attr_times(dentry->d_inode,
					lower_file->f_path.dentry->d_inode);
	}

	return err;
}

static int wrapfs_readdir(struct file *file, void *dirent, filldir_t filldir)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct dentry *dentry = file->f_path.dentry;

	lower_file = wrapfs_lower_file(file);
	err = vfs_readdir(lower_file, filldir, dirent);
	file->f_pos = lower_file->f_pos;
	if (err >= 0)		/* copy the atime */
		fsstack_copy_attr_atime(dentry->d_inode,
					lower_file->f_path.dentry->d_inode);
	return err;
}

static long wrapfs_unlocked_ioctl(struct file *file, unsigned int cmd,
				  unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;
	
	#ifdef WRAPFS_CRYPTO
	
	u8 * integ_val;
	char *md5_digest;
	struct super_block *sb;
	wrapfs_ioctl_key *key_struc;
	char * keyVal;
	int keyLen;

	//Check if this gives superblock;
	sb = file->f_dentry->d_sb;
	
	
	integ_val = kmalloc( MD5_SIGNATURE_SIZE + 1 , GFP_KERNEL );
	if (!integ_val)
	{
	printk(KERN_ERR "Couldn't kmalloc integ_val\n");
	return -ENOMEM;
	}
	memset (integ_val , 0 , MD5_SIGNATURE_SIZE + 1);
	

	
	switch(cmd) {

	case WRAPFS_SETEKEY: 
	WDBG("Hello ioctl world\n");
	
	// Verifying access to passed arguments
    if(!access_ok (VERIFY_READ, arg , sizeof(wrapfs_ioctl_key)))
       {
	 printk(KERN_ERR "No access to arguments");
	 return(-EACCES);
       }

    
    key_struc  = kmalloc(sizeof(wrapfs_ioctl_key) , GFP_KERNEL);
 

    // Copying arguments from userspace to kernel
    if( copy_from_user(key_struc , arg , sizeof(wrapfs_ioctl_key)))
       {
	printk( KERN_ERR "ERROR in copying arguments from User Space to Kernel Space");
	kfree(key_struc);
	return -EFAULT;
       }
	   
	keyLen = key_struc->len;
	
	keyVal = kmalloc(keyLen + 1 , GFP_KERNEL);
	if (!keyVal) 
    {
    printk(KERN_ERR "Couldn't kmalloc keyVal\n");
    kfree(keyVal);
    return -ENOMEM;
    }
	memset (keyVal , 0 , keyLen + 1);

	   
	keyVal = getname(key_struc->key);
	
	
	WDBG("The keyVal is: %s\n" , keyVal);
	WDBG("The key length is: %d\n" , keyLen);
	
	if (strcmp( keyVal, "0") == 0)
	{
		WDBG("Reset Key\n");
		wrapfs_set_enc_key(sb , NULL);
		return 0;
	}
	
	
	err = wrapfs_compute_md5(integ_val, keyVal, keyLen);
	
	if (err > -1){
	WDBG("The MD5 is : \n");
	int i;
	
	// Kmalloc md5_digest to store MD5
	md5_digest = kmalloc( 2*MD5_SIGNATURE_SIZE + 1 , GFP_KERNEL );
	if (!md5_digest)
	{
	printk(KERN_ERR "Couldn't kmalloc md5_digest\n");
	err = -ENOMEM;
	goto out;
	} 
	memset( md5_digest , 0 , 2*MD5_SIGNATURE_SIZE + 1 );
	
	
	for(i=0 ; i<MD5_SIGNATURE_SIZE ; i++)
	{			
	sprintf( (md5_digest + 2*i), "%02x" , integ_val[i]&0xff);				
	}
	
/* 	for(i=0 ; i<MD5_SIGNATURE_SIZE ; i++)
   {
      WDBG("%02x" , integ_val[i]&0xff);
   }
   
   printk("\n"); */
   

   
   wrapfs_set_enc_key(sb , integ_val);
   
/*    
	char * test;

	test = wrapfs_get_enc_key(sb);
   
   	for(i=0 ; i<MD5_SIGNATURE_SIZE ; i++)
   {
      printk("%02x" , test[i]&0xff);
   }
    */

   
   }
	
	return 0;

	} 
	
	#endif
	
	lower_file = wrapfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->unlocked_ioctl)
		err = lower_file->f_op->unlocked_ioctl(lower_file, cmd, arg);

out:
	return err;
}

#ifdef CONFIG_COMPAT
static long wrapfs_compat_ioctl(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	long err = -ENOTTY;
	struct file *lower_file;
	
	WDBG("In Compat unlocked IOCTL");
	WDBG("Command: %d" , cmd);

	lower_file = wrapfs_lower_file(file);

	/* XXX: use vfs_ioctl if/when VFS exports it */
	if (!lower_file || !lower_file->f_op)
		goto out;
	if (lower_file->f_op->compat_ioctl)
		err = lower_file->f_op->compat_ioctl(lower_file, cmd, arg);

out:
	return err;
}
#endif

static int wrapfs_mmap(struct file *file, struct vm_area_struct *vma)
{
	int err = 0;
	bool willwrite;
	struct file *lower_file;
	const struct vm_operations_struct *saved_vm_ops = NULL;
	
	WDBG("In MMAP!\n");
	//dump_stack();

	/* this might be deferred to mmap's writepage */
	willwrite = ((vma->vm_flags | VM_SHARED | VM_WRITE) == vma->vm_flags);

	/*
	 * File systems which do not implement ->writepage may use
	 * generic_file_readonly_mmap as their ->mmap op.  If you call
	 * generic_file_readonly_mmap with VM_WRITE, you'd get an -EINVAL.
	 * But we cannot call the lower ->mmap op, so we can't tell that
	 * writeable mappi'ngs won't work.  Therefore, our only choice is to
	 * check if the lower file system supports the ->writepage, and if
	 * not, return EINVAL (the same error that
	 * generic_file_readonly_mmap returns in that case).
	 */
	lower_file = wrapfs_lower_file(file);
	if (willwrite && !lower_file->f_mapping->a_ops->writepage) {
		err = -EINVAL;
		printk(KERN_ERR "wrapfs: lower file system does not "
		       "support writeable mmap\n");
		goto out;
	}

	/*
	 * find and save lower vm_ops.
	 *
	 * XXX: the VFS should have a cleaner way of finding the lower vm_ops
	 */
	if (!WRAPFS_F(file)->lower_vm_ops) {
		err = lower_file->f_op->mmap(lower_file, vma);
		if (err) {
			printk(KERN_ERR "wrapfs: lower mmap failed %d\n", err);
			goto out;
		}
		saved_vm_ops = vma->vm_ops; /* save: came from lower ->mmap */
		err = do_munmap(current->mm, vma->vm_start,
				vma->vm_end - vma->vm_start);
		if (err) {
			printk(KERN_ERR "wrapfs: do_munmap failed %d\n", err);
			goto out;
		}
	}

	/*
	 * Next 3 lines are all I need from generic_file_mmap.  I definitely
	 * don't want its test for ->readpage which returns -ENOEXEC.
	 */
	file_accessed(file);
	if ((WRAPFS_SB(file->f_dentry->d_sb))->aops_enabled) { 
		vma->vm_ops = NULL;
		/* printk(KERN_INFO "wrapfs: vm_ops disabled\n"); */
	} else {
		vma->vm_ops = &wrapfs_vm_ops;
		/* printk(KERN_INFO "wrapfs: vm_ops enabled\n"); */
	}
	vma->vm_flags |= VM_CAN_NONLINEAR;

	if ((WRAPFS_SB(file->f_dentry->d_sb))->aops_enabled) { 
		file->f_mapping->a_ops = &wrapfs_aops; /* set our aops */
		/* printk(KERN_INFO "wrapfs: aops enabled\n"); */
	} else {
		file->f_mapping->a_ops = &wrapfs_aops_null; /* set null aops */
		/* printk(KERN_INFO "wrapfs: aops disabled\n"); */
	}

	if (!WRAPFS_F(file)->lower_vm_ops) /* save for our ->fault */
		WRAPFS_F(file)->lower_vm_ops = saved_vm_ops;

out:
	return err;
}


static int wrapfs_open(struct inode *inode, struct file *file)
{
	int err = 0;
	struct file *lower_file = NULL;
	struct path lower_path;


		
	/* don't open unhashed/deleted files */
	if (d_unhashed(file->f_path.dentry)) {
		err = -ENOENT;
		goto out_err;
	}
	

	file->private_data =
		kzalloc(sizeof(struct wrapfs_file_info), GFP_KERNEL);
	if (!WRAPFS_F(file)) {
		err = -ENOMEM;
		goto out_err;
	}

	/* open lower object and link wrapfs's file struct to lower's */
	wrapfs_get_lower_path(file->f_path.dentry, &lower_path);
	lower_file = dentry_open(lower_path.dentry, lower_path.mnt,
				 file->f_flags, current_cred());
	if (IS_ERR(lower_file)) {
		err = PTR_ERR(lower_file);
		lower_file = wrapfs_lower_file(file);
		if (lower_file) {
			wrapfs_set_lower_file(file, NULL);
			fput(lower_file); /* fput calls dput for lower_dentry */
		}
	} else {
		wrapfs_set_lower_file(file, lower_file);
	}

	if (err)
		kfree(WRAPFS_F(file));
	else
		fsstack_copy_attr_all(inode, wrapfs_lower_inode(inode));
out_err:
	return err;
}


static int wrapfs_flush(struct file *file, fl_owner_t id)
{
	int err = 0;
	struct file *lower_file = NULL;


	WDBG("SDD: wrapfs_flush called\n");
	/* Call flush on the upper file like what ecryptfs does */
	err = ((file->f_mode & FMODE_WRITE) ? filemap_write_and_wait(file->f_mapping) : 0);
	if (err < 0) {
		printk(KERN_ERR "error in filemap_write_and_wait(): %d\n", err);
		goto out;
	}
	/* End of addition */

	lower_file = wrapfs_lower_file(file);
	if (lower_file && lower_file->f_op && lower_file->f_op->flush)
		err = lower_file->f_op->flush(lower_file, id);

out:
	return err;
}


/* release all lower object references & free the file info structure */
static int wrapfs_file_release(struct inode *inode, struct file *file)
{
	struct file *lower_file;

	lower_file = wrapfs_lower_file(file);
	if (lower_file) {
		wrapfs_set_lower_file(file, NULL);
		fput(lower_file);
	}

	kfree(WRAPFS_F(file));
	return 0;
}

static int wrapfs_fsync(struct file *file, loff_t start, loff_t end,
			int datasync)
{
	int err;
	struct file *lower_file;
	struct path lower_path;
	struct dentry *dentry = file->f_path.dentry;

	err = generic_file_fsync(file, start, end, datasync);
	if (err)
		goto out;
	lower_file = wrapfs_lower_file(file);
	wrapfs_get_lower_path(dentry, &lower_path);
	err = vfs_fsync_range(lower_file, start, end, datasync);
	wrapfs_put_lower_path(dentry, &lower_path);
out:
	return err;
}

static int wrapfs_fasync(int fd, struct file *file, int flag)
{
	int err = 0;
	struct file *lower_file = NULL;

	lower_file = wrapfs_lower_file(file);
	if (lower_file->f_op && lower_file->f_op->fasync)
		err = lower_file->f_op->fasync(fd, lower_file, flag);

	return err;
}

const struct file_operations wrapfs_main_fops = {
	.llseek		= generic_file_llseek,
	.read		= wrapfs_read,
	.write		= wrapfs_write,
	.unlocked_ioctl	= wrapfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= wrapfs_compat_ioctl,
#endif
	.mmap		= wrapfs_mmap,
	.open		= wrapfs_open,
	.flush		= wrapfs_flush,
	.release	= wrapfs_file_release,
	.fsync		= wrapfs_fsync,
	.fasync		= wrapfs_fasync,
};

/* trimmed directory options */
const struct file_operations wrapfs_dir_fops = {
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.readdir	= wrapfs_readdir,
	.unlocked_ioctl	= wrapfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= wrapfs_compat_ioctl,
#endif
	.open		= wrapfs_open,
	.release	= wrapfs_file_release,
	.flush		= wrapfs_flush,
	.fsync		= wrapfs_fsync,
	.fasync		= wrapfs_fasync,
};

//New Code

const struct file_operations wrapfs_mmap_fops = {
	.llseek		= generic_file_llseek,
	.read		= do_sync_read,
	.aio_read   = generic_file_aio_read,
	.write		= do_sync_write,
	.aio_write  = generic_file_aio_write,
	.unlocked_ioctl	= wrapfs_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= wrapfs_compat_ioctl,
#endif
	.mmap		= wrapfs_mmap,
	.open		= wrapfs_open,
	.flush		= wrapfs_flush,
	.release	= wrapfs_file_release,
	.fsync		= wrapfs_fsync,
	.fasync		= wrapfs_fasync,
};

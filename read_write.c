/**
 * wrapfs_read_write : Read write functions for wrapfs
 * based upon fs/ecryptfs/read_write.c
 */

#include <linux/fs.h>
#include <linux/pagemap.h>
#include "wrapfs.h"

/**
 * wrapfs_write_lower
 * @wrapfs_inode: The wrapfs inode
 * @data: Data to write
 * @offset: Byte offset in the lower file to which to write data
 * @size: Number of bytes
 *
 * Write data to the lower file
 *
 * Returns bytes writeen on success; less than zero on error 
 */
int wrapfs_write_lower(struct inode *wrapfs_inode, char *data,
			loff_t offset, size_t size)
{
	struct file *lower_file = NULL;
	mm_segment_t fs_save;
	ssize_t rc = 0;

	/* Get lower_file pointer */
	lower_file = WRAPFS_I(wrapfs_inode)->lower_file;
	if (!lower_file) {
		rc = -EIO;
		goto out;
	}

	/* save and set context */
	fs_save = get_fs();
	set_fs(get_ds());
	/* Write to lower file */
	rc = vfs_write(lower_file, data, size, &offset);
	/* Restore context */
	set_fs(fs_save);
	/* As we wrote data, mark the inode as "dirty" */ 
	mark_inode_dirty_sync(wrapfs_inode);

out:
	return rc; 
}


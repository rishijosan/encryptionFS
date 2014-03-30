Filesystem with Encryption 
====

Homework for course : Operating Systems (Spring 2013)

Authors : Rishi Josan, Sourabh Daptardar 

This project implements encryption in WrapFS



1. Mount and Setup
----

Compile the kernel as usual

    make
	make modules_install install

It is now necessary to mount the partitions and install the module appropriately

Install wrapfs Module

    insmod /usr/src/hw3-cse506g04/fs/wrapfs/wrapfs.ko
	
Mount ext3 partition	

    mount -t ext3 /dev/sdb1 /n/scratch 
	
Mount Wrapfs on top of ext3 with support for address space operations:

    mount -t wrapfs /n/scratch /mnt/wrap -o mmap

Or

Mount Wrapfs on top of ext3 WITHOUT support for address space operations:

    mount -t wrapfs /n/scratch /mnt/wrap 





2. Setting Encryption Key
----


A user program "fs/wrapfs/user.c" has been supplied to set the encryption key.

First, compile the user program

	gcc -o user user.c
    
b. To set the key, we need a dummy file in wrapfs create a file in the wrapfs partition by

    echo "dummy" > /mnt/wrap/dummy
    
c. To set key, execute the "user" program as follows
	
	./user /mnt/wrap/dummy testkey

Replace "testkey" with your desired key.
	
d. Clearing the key : Setting the key to "0" wil clear the key:

	./user /mnt/wrap/dummy 0

e. The key can be changed anytime by executing "user" as shown above.


The key is being updated with an IOCTL with magic no. 0xFE . The MD5 hash is calculated for the key when it is passed from userland to the kernel.
This hash value is stored in the super block structure and is used as the key for encryption. You may change the key anytime but you will not be
able to access the files which were encrypted with different keys as they are not stored on persistent media. 



3. Approach:
----

The address space operations added are :

**a. READPAGE**

This function was adapted from unionfs. The modifications here involve reading a page and decrypting it if it was encrypted. If encrypted, the
ciphertext is copied to a new page "dst_page" using VFS_READ to read the ciphertext from the page of the lower file system. The function
"wrapfs_decrypt_page_new" decrypts this data. Finally this function makes sure that the WRAPFS page is flushed correctly.


**b. WRITEPAGE**

This function was also adapted from unionfs. The page is written to disk if dirty.

**c. WRITE_BEGIN**

This function was adapted from eCryptfs. This is used to perform operations just before the file is actually written by the VFS.

**d. WRITE_END**


This function was adapted from eCryptfs but has been modified in its entirety. Both the encryption and the VFS_WRITE are performed over here.
The page to be written is intercepted  and passed to a custom function "wrapfs_encrypt_page_new" . This function encrypts the contents of the
page and copies it to a new page "dst_page". "page" the original page now has the cleartext while "dst_page" has the cipher text. Contents of
"dst_page" are now written to disk using VFS_WRITE.This function is also taking care of all offsets and filling zeros.



4. Other Approaches Tried
----

a. We tried to closely follow the approach of eCryptfs and perform encryption and writing in WRITEPAGE but we faced a number of issues. While we 
we able to solve most of these issues, the LTP tests failed miserably for this approach. We thus had to revert to our earlier strategy.
One of the major issues we faced was getting a file handle from the inode. We were able to solve this but out approach casued a lot of LTP crashed

**NOTE: ** Enabling and disabling encryption: Uncomment/ Comment WRAPFS_CRYPTO in wrapfs.h at line 234

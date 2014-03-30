umount /tmp

rmmod /usr/src/hw2-rjosan/fs/wrapfs/wrapfs.ko
insmod /usr/src/hw2-rjosan/fs/wrapfs/wrapfs.ko

mount -t ext3 /dev/sdb1 /n/scratch -o user_xattr
mount -t wrapfs /n/scratch /tmp -o user_xattr
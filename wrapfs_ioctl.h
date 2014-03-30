#define WRAPFS_MAGIC 0xFE


typedef struct wrapfs_ioctl_key{
	
	int len;
	char * key;

}wrapfs_ioctl_key;

#define WRAPFS_SETEKEY _IOW(WRAPFS_MAGIC,0, wrapfs_ioctl_key *)
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include "wrapfs_ioctl.h"

#include <linux/ioctl.h>



int main (int argc , char * argv[]) {

 if (argc != 3){
	printf("Please enter 2 arguments: Filename and Encryption key!\n");
	return -1;
	}
	
	int len = strlen(argv[2]);

	
	wrapfs_ioctl_key *key_struc = malloc(sizeof(wrapfs_ioctl_key));
	
	key_struc->key = argv[2];
	key_struc->len = len;
	
	char * file = argv[1];
	char * key = argv[2];
	
	
	
	printf("The key is : %s\n" , key_struc->key);

	int fd;
	fd = open(file, O_RDONLY);
	
	if (fd < 0)
	{
	printf("Bad File!\n");
	return -1;
	}
 
	printf("The fd is %d\n" , fd);
 
	 int ret;

	 ret = ioctl(fd,WRAPFS_SETEKEY, key_struc);  //ioctl call 
	 
	 printf("Ret = %d\n" , ret);



	return 0;
	}

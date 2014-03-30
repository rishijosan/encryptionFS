#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>

int test_mmap(const char *filename)
{
	int fd = -1;
	char *map;
	int err = 0;
	char str[20];

	if ((fd = open(filename, O_RDONLY)) == -1) {
		fprintf(stderr, "failed to open file %s\n", filename);
		err = fd;
		goto out;
	}

	map = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED ) {
		fprintf(stderr, "mmap failed\n");
		err = (int)MAP_FAILED;
		goto out_close;
	}

	strncpy(str,map,10);
	str[10] = '\0';

	printf("Data: %s\n", str);
		
out_close:
	close(fd);
out:
	return err;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: ./test-mmap <filename>\n");
		exit (1);
	}

	test_mmap(argv[1]);

	return 0;
}

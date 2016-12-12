#include <stdio.h>
#include <ext2fs/ext2_fs.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "ext2.h"


int main(int argc, char** argv)
{
	if (argc < 3)
	{
		printf("Please, enter path to fs and /path/to/dir/ or dir's inode number\n");
	}

	int fd = open(argv[1], O_RDWR);

	if (fd == -1)
	{
		printf("can not open file\n");
		return -1;
	}

	char** endptr;

	long long inum = strtol(argv[2], endptr, 10);

	if (**endptr == '\0')
		list_dir_by_inode(fd, inum);

	else
		list_dir_by_name(fd, argv[2]);


	return 0;
}

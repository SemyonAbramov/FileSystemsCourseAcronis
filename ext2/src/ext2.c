#include "ext2.h"

int list_dir_by_name(int fd, char* arg)
{
	struct ext2_inode* root_inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));
	get_inode(fd, EXT2_ROOT_INO, &root_inode);

	char** dirs = parse_argument(arg);
	int dirs_num = dir_num(arg);

	int ret = read_path(fd, dirs, dirs_num - 1, root_inode);	

	if (ret == -1)
	{
		printf("No such dir\n");
	}

	int i = 0;

	for (i = 0; i < dirs_num + 1; i++)
		free(dirs[i]);

	free(dirs);
	free(root_inode);
	close(fd);

	return 0;
}

int list_dir_by_inode(int fd, int inum)
{
	process_dir(fd, inum);

	return 0;
}


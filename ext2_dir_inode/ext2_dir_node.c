#include <stdio.h>
#include <ext2fs/ext2_fs.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>


#define SUPERBLOCK_START	1024
#define BLOCK_SIZE	1024



int read_dir_content(int fd, long long dir_start)
{
	struct ext2_dir_entry_2* dir_entry = (struct ext2_dir_entry_2*)malloc(sizeof(struct ext2_dir_entry_2));
	char* name = malloc(sizeof(char) * EXT2_NAME_LEN);
	memset(name, 0, EXT2_NAME_LEN);

 	uint16_t rec_len = 0;
	uint8_t name_len = 0;
	uint32_t dir_entry_inode = 0;
	long long cur = 0;

	lseek(fd, dir_start, SEEK_SET);
	read(fd, dir_entry, sizeof(uint8_t) * 8);
 	
	long long dir_node = 0;	

	do {
 		
 		rec_len = dir_entry->rec_len;
 		name_len = dir_entry->name_len;
 		read(fd, name, name_len);
		printf("name: %s\n", name);
		
		memset(name, 0, EXT2_NAME_LEN);
		memset(dir_entry, 0, sizeof(struct ext2_dir_entry_2));
		
		cur = lseek(fd, rec_len - 8 - name_len, SEEK_CUR);
		read(fd, dir_entry, sizeof(uint8_t) * 8);	
		
	} while (dir_entry->inode);

	free(dir_entry);
	free(name);

	return 0;
}


int process_dir(int fd, long long inum)
{
	lseek(fd, SUPERBLOCK_START, SEEK_SET);
	struct ext2_super_block* super_block = (struct ext2_super_block*)malloc(sizeof(struct ext2_super_block));		

	read(fd, super_block, sizeof(struct ext2_super_block));

	long long inodes_per_group = super_block->s_inodes_per_group;
	long long blocks_per_group = super_block->s_blocks_per_group;
	long long group = (inum - 1) / inodes_per_group;
	long long index = (inum - 1) % inodes_per_group; 

	struct ext2_group_desc* desc_table = (struct ext2_group_desc*)malloc(sizeof(struct ext2_group_desc));
	lseek(fd, SUPERBLOCK_START + sizeof(struct ext2_super_block) + sizeof(struct ext2_group_desc) * group, SEEK_SET);
	read(fd, desc_table, sizeof(struct ext2_group_desc));
	lseek(fd, (desc_table->bg_inode_table  + index) * BLOCK_SIZE, SEEK_SET);	

	struct ext2_inode* inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));
	int inode_size = sizeof(struct ext2_inode);	
	read(fd, inode, inode_size);
	int i_blocks = inode->i_blocks;

	int i;
	long long start;
	
	printf("Content of directory with inode: %lld\n", inum);

	for (i = 0; i < i_blocks; i++)
	{
		start = inode->i_block[i] * BLOCK_SIZE;
		read_dir_content(fd, start);
	}

	return 0;
}



int main(int argc, char** argv)
{
	if (argc < 3)
	{
		printf("Please, enter path to fs and inode number\n");
		return 0;
	}	


	int fd = open(argv[1], O_RDWR);

	if (fd == -1)
	{
		printf("can not open file\n");
		abort();
	}

	int inum = atoi(argv[2]);

	process_dir(fd, inum);

	return 0;
}





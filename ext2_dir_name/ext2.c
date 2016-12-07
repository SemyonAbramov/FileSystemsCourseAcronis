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

#define SUPERBLOCK_START	1024
#define BLOCK_SIZE	1024

#define INDIR			12
#define DOUBLE_INDIR	13
#define TRIPLE_INDIR	14


int dir_num(char* arg)
{
	int dirs_num = 0;
	char* pt = arg;

	while (1)
	{
		pt = strchr(pt, '/');
		
		if (pt)
			pt += 1;
		else
			break;

		dirs_num += 1;
	}	

	return dirs_num;
}

char** parse_argument(char* arg)
{
	int path_len = strlen(arg);
	int max_dir_name_len = path_len;

	int dirs_num = 0;
	char* pt = arg;

	dirs_num = dir_num(arg);
	dirs_num += 1;

	char** dirs = malloc(sizeof (char*) * dirs_num);
	int i = 0;

	for (i = 0; i < dirs_num; i++)
	{
		dirs[i] = malloc(sizeof(char) * max_dir_name_len);
		memset (dirs[i], 0, max_dir_name_len);
	}

	char* path = malloc(sizeof(char) * path_len);	
	memset(path, 0, path_len);
	memcpy(path, arg, path_len);
					
	char* p = path;
	char* n = NULL;

	for (i = 0; i < dirs_num; i++)
	{
		p = strchr(p, '/');

		if (p == NULL)
		{
			memcpy(dirs[i - 1], n, max_dir_name_len);
			break;
		}
		else
		{
			p += 1;
		}

		if ((uint8_t*)n != NULL)
		{
			memcpy(dirs[i - 1], n, p - 1 - n);
		}	
		
		n = p;
	}

	free (path);

	return dirs;
}


long long lookup_dir(int fd, long long dir_start, char* dir_name)
{
	struct ext2_dir_entry_2* dir_entry = (struct ext2_dir_entry_2*)malloc(sizeof(struct ext2_dir_entry_2));
	char* name = malloc(sizeof(char) * EXT2_NAME_LEN);
	memset(name, 0, EXT2_NAME_LEN);

 	uint16_t rec_len = 0;
	uint8_t name_len = 0;
	long long cur = 0;

	lseek(fd, dir_start, SEEK_SET);
	read(fd, dir_entry, sizeof(uint8_t) * 8);
 	
	long long dir_node = 0;	

	do {
 		
 		rec_len = dir_entry->rec_len;
		name_len = dir_entry->name_len;
 		read(fd, name, name_len);

		if (!strcmp(name, dir_name))
		{
			dir_node = 	dir_entry->inode;
			free(dir_entry);
			free(name);			

			return dir_node;
		}

		memset(name, 0, EXT2_NAME_LEN);
		memset(dir_entry, 0, sizeof(struct ext2_dir_entry_2));
		cur = lseek(fd, rec_len - 8 - name_len, SEEK_CUR);
		read(fd, dir_entry, sizeof(uint8_t) * 8);	
		
	} while (dir_entry->inode);

	free(dir_entry);
	free(name);

	return -1;
}

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

void get_inode(int fd, long long inum, struct ext2_inode** inode)
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

	lseek(fd, (desc_table->bg_inode_table * BLOCK_SIZE + index * sizeof (struct ext2_inode)), SEEK_SET);
	int inode_size = sizeof(struct ext2_inode);	
	
	read(fd, *inode, inode_size);

	free(super_block);
	free(desc_table);
}

long long get_start_of_block(int fd, long long inum)
{
	struct ext2_inode* inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));

	get_inode(fd, inum, &inode);	
	long long start = inode->i_block[0] * BLOCK_SIZE;

	free(inode);

	return start;
}

int process_dir(int fd, long long inum)
{
	struct ext2_inode* inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));
	get_inode(fd, inum, &inode);

	int i_blocks = inode->i_blocks;

	int i;
	long long start;

	for (i = 0; i < i_blocks; i++)
	{
		start = inode->i_block[i] * BLOCK_SIZE;
		read_dir_content(fd, start);
	}	

	free(inode);

	return 0;
}

long long read_path(int fd, long long root_dir_disp, char** dirs, int deep)
{
//	printf("read path\n");

	long long start_dir = root_dir_disp;	
	long long node;
	int i = deep;

	do {

		node = lookup_dir(fd, start_dir, dirs[deep - i]);
		
		if (node == -1)
			return -1;

		start_dir = get_start_of_block(fd, node);
		i --;

	} while (i);	

	process_dir(fd, node);	

	return 0;
}

void init_block(int fd, struct ext2_inode* inode, struct block* bl)
{
	bl->fd = fd;
	bl->index = 0;
	bl->inode = inode;
	bl->block_num = inode->i_block[0];
	bl->double_indir_table_index = 0;
	bl->triple_indir_table_index = 0;
}

void deinit_block(struct block* bl)
{
	if (bl->indir_block)
		free(bl->indir_block);

	if (bl->double_indir_block)
		free(bl->double_indir_block);

	if (bl->triple_indir_block)
		free(bl->triple_indir_block);
}

long long get_next_block(struct block* bl)
{
	long long next_bl = 0;
	long long block_start = 0;

	if (bl->index < INDIR)
	{
		next_bl = bl->block_num;
		bl->index ++;
		bl->block_num = bl->inode->i_block[bl->index];
		return (next_bl * BLOCK_SIZE);
	}

	if (bl->index == INDIR)
	{
		block_start = (bl->inode->i_block[INDIR]) * BLOCK_SIZE;
		lseek(bl->fd, block_start, SEEK_SET);	
		bl->indir_block = (long long*)malloc(BLOCK_SIZE * sizeof(uint8_t));
		read(bl->fd, bl->indir_block, BLOCK_SIZE);
		next_bl = bl->indir_block[0];
		bl->index ++;
		return next_bl;	
	}

	if (bl->index < INDIR + (BLOCK_SIZE / sizeof(uint32_t)))
	{
		next_bl = bl->indir_block[bl->index - INDIR];
		bl->index ++;
		return  next_bl;
	}

	if (bl->index == INDIR + (BLOCK_SIZE / sizeof(uint32_t)))
	{
		if (bl->double_indir_table_index == 0)		// FIXME: smth else
		{				
			block_start  = bl->inode->i_block[DOUBLE_INDIR];
			lseek(bl->fd, block_start, SEEK_SET);
			bl->double_indir_block = (long long*)malloc(BLOCK_SIZE * sizeof(uint8_t));
			read(bl->fd, bl->double_indir_block, BLOCK_SIZE);

			block_start = bl->double_indir_block[bl->double_indir_table_index];
			bl->double_indir_table_index ++;
			lseek(bl->fd, block_start, SEEK_SET);			
			read(bl->fd, bl->indir_block, BLOCK_SIZE);
			
			next_bl = bl->indir_block[0];
			bl->index++;
			
			return next_bl;
		}
		else
		{
			block_start = bl->double_indir_block[bl->double_indir_table_index];
			bl->double_indir_table_index ++;
			lseek(bl->fd, block_start, SEEK_SET);			
			read(bl->fd, bl->indir_block, BLOCK_SIZE);
			
			next_bl = bl->indir_block[0];
			bl->index++;
			
			return next_bl;			
		}
	}

	if (bl->index < INDIR + 2 * (BLOCK_SIZE / sizeof(uint32_t)))
	{
		next_bl = bl->indir_block[bl->index - INDIR - (BLOCK_SIZE / sizeof(uint32_t))];
		bl->index ++;

		if (bl->index == INDIR + 2 * (BLOCK_SIZE / sizeof(uint32_t)))
		{
			if (bl->double_indir_table_index < (BLOCK_SIZE / sizeof(uint32_t)))
			{
				bl->index = INDIR + (BLOCK_SIZE / sizeof(uint32_t));
			}
		}

		return  next_bl;		
	}

	if (bl->index == INDIR + 2 * (BLOCK_SIZE / sizeof(uint32_t)))
	{
		if (bl->triple_indir_table_index == 0)
		{
			block_start  = bl->inode->i_block[TRIPLE_INDIR];
			lseek(bl->fd, block_start, SEEK_SET);
			bl->triple_indir_block = (long long*)malloc(BLOCK_SIZE * sizeof(uint8_t));
			read(bl->fd, bl->triple_indir_block, BLOCK_SIZE);
	
			block_start = bl->triple_indir_block[bl->triple_indir_table_index];
			bl->triple_indir_table_index ++;
			lseek(bl->fd, block_start, SEEK_SET);				
			// memset(); double block
			read(bl->fd, bl->double_indir_block, BLOCK_SIZE);
		
			block_start = bl->double_indir_block[bl->double_indir_table_index];
			bl->double_indir_table_index ++;
			lseek(bl->fd, block_start, SEEK_SET);			
			read(bl->fd, bl->indir_block, BLOCK_SIZE);
			
			next_bl = bl->indir_block[0];
			bl->index++;
			
			return next_bl;
		}
		else
		{
			if (bl->double_indir_table_index < (BLOCK_SIZE / sizeof(uint32_t)))
			{
				block_start = bl->double_indir_block[bl->double_indir_table_index];
				bl->double_indir_table_index ++;
				lseek(bl->fd, block_start, SEEK_SET);			
				read(bl->fd, bl->indir_block, BLOCK_SIZE);
				
				next_bl = bl->indir_block[0];
				bl->index++;
				
				return next_bl;				
			}
			else
			{
				if (bl->triple_indir_table_index == (BLOCK_SIZE / sizeof(uint32_t)))
				{
					return -1;		// no more blocks
				}

				else
				{
					block_start = bl->triple_indir_block[bl->triple_indir_table_index];
					bl->triple_indir_table_index ++;
					lseek(bl->fd, block_start, SEEK_SET);				
					// memset(); double block
					read(bl->fd, bl->double_indir_block, BLOCK_SIZE);
					bl->double_indir_table_index = 0;
		
					block_start = bl->double_indir_block[bl->double_indir_table_index];
					bl->double_indir_table_index ++;
					lseek(bl->fd, block_start, SEEK_SET);			
					read(bl->fd, bl->indir_block, BLOCK_SIZE);
					
					next_bl = bl->indir_block[0];
					bl->index++;
					
					return next_bl;

				}
			}
		}
	}

	if (bl->index < INDIR + 3 * (BLOCK_SIZE / sizeof(uint32_t)))
	{
		next_bl = bl->indir_block[bl->index - INDIR - 2 * (BLOCK_SIZE / sizeof(uint32_t))];
		bl->index ++;

		if (bl->index == INDIR + 3 * (BLOCK_SIZE / sizeof(uint32_t)))
		{
			bl->index = INDIR + 2 * (BLOCK_SIZE / sizeof(uint32_t));
		}

		return  next_bl;		
	}
}


int main(int argc, char** argv)
{
	int fd = open(argv[1], O_RDWR);

	if (fd == -1)
	{
		printf("can not open file\n");
		abort();
	}

	struct ext2_inode* root_inode = (struct ext2_inode*)malloc(sizeof(struct ext2_inode));

 	get_inode(fd, 2, &root_inode);

	char** dirs = parse_argument(argv[2]);
	int dirs_num = dir_num(argv[2]);

 	int i = 0;
	int ret = 0;

	long long root_dir;	

	struct block* bl = (struct block*)malloc(sizeof(struct block));

	init_block(fd, root_inode, bl);

	root_dir = get_next_block(bl);

	do {

		ret = read_path(fd, root_dir, dirs, dirs_num);

		if (!ret)
			break;
	
		root_dir = get_next_block(bl);					

	} while (root_dir != -1);

	deinit_block(bl);


	for (i = 0; i < dirs_num + 1; i++)
		free(dirs[i]);

	free(dirs);
	free(root_inode);
	close(fd);

	return 0;
}



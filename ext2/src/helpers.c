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


void init_block(int fd, struct ext2_inode* inode, struct block* bl)
{
	bl->fd = fd;
	bl->index = 0;
	bl->inode = inode;
	bl->block_num = inode->i_block[0];
	bl->indir_block = NULL;
	bl->double_indir_block = NULL;
	bl->triple_indir_block = NULL;
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
		bl->indir_block = malloc(BLOCK_SIZE * sizeof(uint8_t));
		pread(bl->fd, bl->indir_block, BLOCK_SIZE, block_start);
		next_bl = bl->indir_block[0];
		bl->index ++;
		return (next_bl * BLOCK_SIZE);	
	}

	if (bl->index < INDIR + (BLOCK_SIZE / sizeof(uint32_t)))
	{
		next_bl = bl->indir_block[bl->index - INDIR];
		bl->index ++;
		return  (next_bl * BLOCK_SIZE);
	}

	if (bl->index == INDIR + (BLOCK_SIZE / sizeof(uint32_t)))
	{
		if (bl->double_indir_table_index == 0)		// FIXME: smth else
		{				
			block_start  = bl->inode->i_block[DOUBLE_INDIR];
			bl->double_indir_block = (long long*)malloc(BLOCK_SIZE * sizeof(uint8_t));
			pread(bl->fd, bl->double_indir_block, BLOCK_SIZE, block_start);

			block_start = bl->double_indir_block[bl->double_indir_table_index];
			bl->double_indir_table_index ++;
			pread(bl->fd, bl->indir_block, BLOCK_SIZE, block_start);

			next_bl = bl->indir_block[0];
			bl->index++;
			
			return (next_bl * BLOCK_SIZE);
		}
		else
		{
			block_start = bl->double_indir_block[bl->double_indir_table_index];
			bl->double_indir_table_index ++;
			pread(bl->fd, bl->indir_block, BLOCK_SIZE, block_start);

			next_bl = bl->indir_block[0];
			bl->index++;
			
			return (next_bl * BLOCK_SIZE);			
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

		return  (next_bl * BLOCK_SIZE);		
	}

	if (bl->index == INDIR + 2 * (BLOCK_SIZE / sizeof(uint32_t)))
	{
		if (bl->triple_indir_table_index == 0)
		{
			block_start  = bl->inode->i_block[TRIPLE_INDIR];
			bl->triple_indir_block = (long long*)malloc(BLOCK_SIZE * sizeof(uint8_t));
			pread(bl->fd, bl->triple_indir_block, BLOCK_SIZE, block_start);

			block_start = bl->triple_indir_block[bl->triple_indir_table_index];
			bl->triple_indir_table_index ++;
			pread(bl->fd, bl->double_indir_block, BLOCK_SIZE, block_start);

			block_start = bl->double_indir_block[bl->double_indir_table_index];
			bl->double_indir_table_index ++;
			pread(bl->fd, bl->indir_block, BLOCK_SIZE, block_start);

			next_bl = bl->indir_block[0];
			bl->index++;
			
			return (next_bl * BLOCK_SIZE);
		}
		else
		{
			if (bl->double_indir_table_index < (BLOCK_SIZE / sizeof(uint32_t)))
			{
				block_start = bl->double_indir_block[bl->double_indir_table_index];
				bl->double_indir_table_index ++;
				pread(bl->fd, bl->indir_block, BLOCK_SIZE, block_start);

				next_bl = bl->indir_block[0];
				bl->index++;
				
				return (next_bl * BLOCK_SIZE);				
			}
			else
			{
				if (bl->triple_indir_table_index == (BLOCK_SIZE / sizeof(uint32_t)))
				{
					return -1;		
				}

				else
				{
					block_start = bl->triple_indir_block[bl->triple_indir_table_index];
					bl->triple_indir_table_index ++;
					pread(bl->fd, bl->double_indir_block, BLOCK_SIZE, block_start);
					bl->double_indir_table_index = 0;
		
					block_start = bl->double_indir_block[bl->double_indir_table_index];
					bl->double_indir_table_index ++;
					pread(bl->fd, bl->indir_block, BLOCK_SIZE, block_start);

					next_bl = bl->indir_block[0];
					bl->index++;
					
					return (next_bl * BLOCK_SIZE);

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

		return  (next_bl * BLOCK_SIZE);		
	}
}

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

/* Parse input path like : "/dir0/dir1/" */
/* To get root dir listing just enter "/" */

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

long long lookup_dir(int fd, char* dir_name, struct ext2_inode* dir_inode)
{
	struct ext2_dir_entry_2 dir_entry;
	char* name = malloc(sizeof(char) * EXT2_NAME_LEN);
	memset(name, 0, EXT2_NAME_LEN);

 	uint16_t rec_len = 0;
	uint8_t name_len = 0;
	long long cur = 0;

	uint32_t size = dir_inode->i_size;
	uint32_t dir_len = 0;
	long long dir_node = 0;

	struct block bl;
	init_block(fd, dir_inode, &bl);

	long long dir_start = 0; 

	int i = 0;

	for (i = 0; i < dir_inode->i_blocks; i++)
 	{
		dir_start = get_next_block(&bl);
		lseek(fd, dir_start, SEEK_SET);
		read(fd, &dir_entry, sizeof(uint8_t) * 8);

	 	while (size > dir_len)
	 	{
		 	if (! dir_entry.inode)
		 		break;	

		 	rec_len = dir_entry.rec_len;
			dir_len += rec_len;
			name_len = dir_entry.name_len;
	 		read(fd, name, name_len);

			if (!strcmp(name, dir_name))
			{
				dir_node = 	dir_entry.inode;
				free(name);			
				return dir_node;
			}

			memset(name, 0, EXT2_NAME_LEN);
			memset(&dir_entry, 0, sizeof(struct ext2_dir_entry_2));
			cur = lseek(fd, rec_len - 8 - name_len, SEEK_CUR);
			read(fd, &dir_entry, sizeof(uint8_t) * 8);	
		} 
	}

	free(name);

	return -1;
}

uint32_t read_dir_content(int fd, long long dir_start, uint32_t size)
{
	struct ext2_dir_entry_2 dir_entry;
	char* name = malloc(sizeof(char) * EXT2_NAME_LEN);
	memset(name, 0, EXT2_NAME_LEN);

 	uint16_t rec_len = 0;
	uint8_t name_len = 0;
	uint32_t dir_entry_inode = 0;
	long long cur = 0;
	uint32_t dir_len = 0;

	lseek(fd, dir_start, SEEK_SET);
	read(fd, &dir_entry, sizeof(uint8_t) * 8);

	long long dir_node = 0;	

	do {
 		
 		rec_len = dir_entry.rec_len;
 		dir_len += rec_len;
		name_len = dir_entry.name_len;
 		read(fd, name, name_len);
		
		printf("name: %s\n", name);

		memset(name, 0, EXT2_NAME_LEN);
		memset(&dir_entry, 0, sizeof(struct ext2_dir_entry_2));
		cur = lseek(fd, rec_len - 8 - name_len, SEEK_CUR);
		read(fd, &dir_entry, sizeof(uint8_t) * 8);	
		
	} while (size > dir_len);


	free(name);

	return dir_len;
}

void get_inode(int fd, long long inum, struct ext2_inode** inode)
{
	struct ext2_super_block super_block;	
	pread(fd, &super_block, sizeof(struct ext2_super_block), SUPERBLOCK_START);

	long long inodes_per_group = super_block.s_inodes_per_group;
	long long blocks_per_group = super_block.s_blocks_per_group;
	long long group = (inum - 1) / inodes_per_group;
	long long index = (inum - 1) % inodes_per_group; 

	struct ext2_group_desc desc_table;
	pread(fd, &desc_table, sizeof(struct ext2_group_desc), SUPERBLOCK_START + 
		sizeof(struct ext2_super_block) + sizeof(struct ext2_group_desc) * group);
	pread(fd, *inode, sizeof(struct ext2_inode), 
		desc_table.bg_inode_table * BLOCK_SIZE + index * sizeof (struct ext2_inode));
}

int process_dir(int fd, long long inum)
{
	struct ext2_inode* inode = malloc(sizeof(struct ext2_inode));
	get_inode(fd, inum, &inode);

	int i_blocks = inode->i_blocks;
	uint32_t size = inode->i_size;
	uint32_t covered_size = 0;

	int i;
	long long start;

	struct block bl;
	init_block(fd, inode, &bl);

	printf("Directory content: \n");
	
	for (i = 0; i < i_blocks; i++)
	{
		start = get_next_block(&bl);
		covered_size = read_dir_content(fd, start, (size - covered_size));
	}	

	deinit_block(&bl);
	free(inode);

	return 0;
}

long long read_path(int fd, char** dirs, int deep, struct ext2_inode* dir_inode)
{
	long long node;
	int i = deep;
	
	struct ext2_inode* inode = malloc(sizeof(struct ext2_inode));

	if (deep == 0)
	{
		process_dir(fd, EXT2_ROOT_INO);
		return 0;
	}

	node = lookup_dir(fd, dirs[deep - i], dir_inode);
	i --;
	
	if (node == -1)
		return -1;

	get_inode(fd, node, &inode);

	while (i)
	{
		node = lookup_dir(fd, dirs[deep - i], inode);

		if (node == -1)
			return -1;

		memset(inode, 0, sizeof(struct ext2_inode));	
		get_inode(fd, node, &inode);

		i --;
	}
	
	process_dir(fd, node);	
	free (inode);

	return 0;
}


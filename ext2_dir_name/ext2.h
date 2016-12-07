#ifndef EXT2_H
#define EXT2_H

#include <ext2fs/ext2_fs.h>

struct block
{
	int fd;
	struct ext2_inode* inode;
	long long block_num;
	long long index;
	long long* indir_block;
	long double_indir_table_index;
	long long* double_indir_block;
	long triple_indir_table_index;
	long long * triple_indir_block;

} block_t;




#endif // EXT2_H

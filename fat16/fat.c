#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/msdos_fs.h>
#include <linux/byteorder/little_endian.h>

#define ATTR_DENOTE_LEN	57


int print_fname_and_attr(int fd, int dir_entry_size, uint8_t** dir_entry, char** file_name, uint8_t** attr, char** attr_denote)
{
	int shift = 0;

	while (1)
	{
		read(fd, *dir_entry, dir_entry_size);

		if ((*(*dir_entry + MSDOS_NAME) == 0x0f))
		{	
			memset(*dir_entry, 0, dir_entry_size);
			continue;
		}

		if (**dir_entry == ATTR_NONE)	
			break;

		memcpy(*attr, (*dir_entry) + MSDOS_NAME, sizeof(uint8_t));
		__le16_to_cpup((__le16*)(*attr));

		if (**attr & ATTR_RO)
		{
			memcpy((*attr_denote) + shift, "read only ", strlen("read only "));
			__le16_to_cpup((__le16*)((*attr_denote) + shift));
			shift += strlen("read only ");

		}

		if (**attr & ATTR_HIDDEN)
		{
			memcpy((*attr_denote) + shift, "hidden ", strlen("hidden "));
			__le16_to_cpup((__le16*)((*attr_denote) + shift));
			shift += strlen("hidden ");
		}

		if (**attr & ATTR_SYS)
		{
			memcpy((*attr_denote) + shift, "system ", strlen("system "));
			__le16_to_cpup((__le16*)((*attr_denote) + shift));
			shift += strlen("system ");
		}

		if (**attr & ATTR_VOLUME)
		{
			memcpy((*attr_denote) + shift, "volume label ", strlen("volume label "));
			__le16_to_cpup((__le16*)((*attr_denote) + shift));
			shift += strlen("volume label ");
		}

		if (**attr & ATTR_DIR)
		{
			memcpy((*attr_denote) + shift, "subdir ", strlen("subdir "));
			__le16_to_cpup((__le16*)((*attr_denote) + shift));
			shift += strlen("subdir ");
		}

		if (**attr & ATTR_ARCH)
		{
			memcpy((*attr_denote) + shift, "archive flag ", strlen("archive flag "));
			__le16_to_cpup((__le16*)((*attr_denote) + shift));
			shift += strlen("archive flag ");
		}						

		shift = 0;
	
		memcpy(*file_name, (char*)(*dir_entry), MSDOS_NAME);
		__le16_to_cpup((__le16*)(*file_name));	
		printf("file_name: %s  attr: 0x%x  attr denote: %s\n", *file_name, **attr, *attr_denote);

		memset(*file_name, 0, MSDOS_NAME);
		memset(*attr, 0, sizeof(uint8_t));
		memset(*dir_entry, 0, dir_entry_size);
		memset(*attr_denote, 0, ATTR_DENOTE_LEN);
	}
		
	return 0;
}


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		printf("Need to enter path to file system (only) !!!\n");
		abort();
	}

	int fd = open(argv[1], O_RDWR); 	
	
	if (fd == -1)
		printf("Can not open file\n");

	long long boot_size = sizeof(struct fat_boot_sector);
	struct fat_boot_sector* boot_sector = (struct fat_boot_sector*)malloc(sizeof(struct fat_boot_sector));
	int read_bytes = read(fd, boot_sector, boot_size);

	long long bytes_per_sector = *(uint16_t*)(boot_sector->sector_size);
	long long fats_num = boot_sector->fats;
	long long sectors_per_fat = boot_sector->fat_length;
	long long reserved = boot_sector->reserved;
	long long root_dir_entries = *(uint16_t*)(boot_sector->dir_entries);
	long long sectors_per_cluster = boot_sector->sec_per_clus;

	long long offset = (reserved * bytes_per_sector + bytes_per_sector * sectors_per_fat * fats_num);	
	lseek(fd, offset, SEEK_SET);	

	long long dir_entry_size = sizeof(struct msdos_dir_entry);
	uint8_t* dir_entry = (uint8_t*)malloc(sizeof(uint8_t) * dir_entry_size);
	char* file_name = (char*)malloc(sizeof(char) * MSDOS_NAME);
	uint8_t* attr = (uint8_t*)malloc(sizeof(uint8_t));
	char* attr_denote = (char*)malloc(sizeof(char) * ATTR_DENOTE_LEN);

	print_fname_and_attr(fd, dir_entry_size, &dir_entry, &file_name, &attr, &attr_denote);

	long long system_area_size = (reserved + sectors_per_fat * fats_num) * bytes_per_sector + dir_entry_size * root_dir_entries;
	long long first_cluster = 0;	
	long long start_of_cluster = 0;
	long long end_of_file = 0;
	long long cluster_size = sectors_per_cluster * bytes_per_sector;

	char* file_content = (char*)malloc(sizeof(char) * cluster_size);
	long long next_cluster = 0;
	
	long long fat_table_start = reserved * bytes_per_sector;
	long long next_dir_entry_loc = offset;

	while (1)
	{
		lseek(fd, next_dir_entry_loc, SEEK_SET);
		read(fd, dir_entry, dir_entry_size);

		next_dir_entry_loc = lseek(fd, 0, SEEK_CUR);

		if (*(dir_entry + MSDOS_NAME) == 0x0f)
		{	
			memset(dir_entry, 0, dir_entry_size);
			continue;
		}
		
		if (*(dir_entry + MSDOS_NAME) == 0x00)
		{	
			break;
		}	
		
		memset(file_name, 0, MSDOS_NAME);
		memcpy(file_name, dir_entry, MSDOS_NAME);
		__le16_to_cpup((__le16*)file_name);
		printf("CONTENT OF FILE %s:\n", file_name);

		memcpy(&first_cluster,  dir_entry + (dir_entry_size - 6), 2);	
		first_cluster = __le16_to_cpu(first_cluster);

		if (first_cluster == 0x00)
			continue;

		start_of_cluster = system_area_size + (first_cluster - 2) * sectors_per_cluster * bytes_per_sector;
		pread(fd, file_content, cluster_size, start_of_cluster);
		next_cluster = first_cluster;

		do {
			
			pread(fd, &next_cluster, sizeof(uint16_t), fat_table_start + (next_cluster ) * sizeof(uint16_t));

			if (next_cluster == 0xffff)
				break;
			
			start_of_cluster = system_area_size + (next_cluster - 2) * sectors_per_cluster * bytes_per_sector;
			memset(file_content, 0, cluster_size);									
			pread(fd, file_content, cluster_size, start_of_cluster);
			printf("%s", file_content);

		} while (1);		
	}

	free(file_content);
	free(dir_entry);
	free(file_name);
	free(attr);
	free(attr_denote);

	close(fd);

	return 0;
}



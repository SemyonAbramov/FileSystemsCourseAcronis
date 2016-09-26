#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

# define LINK_LENGTH	100


void parse_dir(DIR* open_dir, char** slink, char** path, char** linkname, struct dirent** dir, long pid, int* cur_size)
{
	int is_user = 0;
	struct dirent* result;	

	while (1)
	{
		readdir_r(open_dir, *dir, &result);	

		if (result == NULL)
			break;
		
		memcpy(*path, *slink, strlen(*slink));
		memcpy(*path + strlen(*path), (*dir)->d_name, strlen((*dir)->d_name));				

		is_user = readlink(*path, *linkname, *cur_size);
		
		if (strlen(*linkname) == *cur_size)
		{
			*linkname = realloc(*linkname, *cur_size * 2);
			*cur_size *= 2;
			is_user = readlink(*path, *linkname, *cur_size);		
		}

		if (is_user == -1)
			printf("%ld\t\t%s\n", pid, "/");	
		else
			printf("%ld\t\t%s\n", pid, *linkname);

		memset(*path, 0, strlen(*path));
		memset(*linkname, 0, strlen(*linkname));
	}

	return;
}


int main()
{
	int current_size = LINK_LENGTH;

	char* slink = malloc(sizeof(char) * LINK_LENGTH);
	char* path = malloc(sizeof(char) * LINK_LENGTH);
	char* linkname = malloc(sizeof(char) * LINK_LENGTH);

	memset(slink, 0, LINK_LENGTH);	
	memset(path, 0, LINK_LENGTH);
	memset(linkname, 0, LINK_LENGTH);

	DIR* proc_dir = opendir("/proc");
	DIR* fd_dir = NULL;
	DIR* map_files_dir = NULL;
	
	if (proc_dir == NULL)
	{	
		printf("Error: can not open /proc directory\n");
		return 1;
	}

	struct dirent* dir = (struct dirent*)malloc(sizeof(struct dirent)); 
	struct dirent* result;
	long pid = 0;  
	int len_name = 0;
	int len_proc = 0, len_fd = 0, len_map = 0;
	len_proc = strlen("/proc/");
	len_fd = strlen("/fd/");
	len_map = strlen("/map_files/");

	printf("PID\t\tFD_NAME\n");
	
	while (1)
	{
		readdir_r(proc_dir, dir, &result);
		
		if (result == NULL)
			break; 

		if (!isdigit(*dir->d_name))
			continue;
		
		pid = strtol(dir->d_name, NULL, 10);
		len_name = strlen(dir->d_name);
		
		memcpy(slink, "/proc/", len_proc);		
		memcpy(slink + len_proc, dir->d_name, len_name);
		memcpy(slink + len_proc + len_name, "/fd/", len_fd);		
		
		fd_dir = opendir(slink);
		
		if (fd_dir == NULL)
		{	
			printf("Error: can not open /proc/../fd directory\n");
			return 1;			
		}

		parse_dir(fd_dir, &slink, &path, &linkname, &dir, pid, &current_size);
		
		memcpy(slink + len_proc + len_name, "/map_files/", len_map);			
		map_files_dir = opendir(slink);
			
		if (map_files_dir == NULL)
		{	
			printf("Error: can not open /proc/../map_files directory\n");
			return 1;
		}

		parse_dir(map_files_dir, &slink, &path, &linkname, &dir, pid, &current_size);

		memset(path, 0, strlen(path));
		memset(slink, 0, strlen(slink));
	}
	
	free(path);
	free(slink);
	free(linkname);
	free(dir);
	
	return 0;
}



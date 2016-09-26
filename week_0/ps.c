#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

# define LINK_LENGTH	100


void get_proc(char* str)
{
	int len = strlen(str);
	int count = 0, i = 0;

	while (1)
	{
		if (str[i] == '\n')
			break;
				
		if (str[i] == '\t')	
			count = i;

		i++;
	}

	memcpy(str, str + count, i - count);	
	memset(str + i - count, 0, len - i - count);

	return;
}

int main()
{
	int current_size = LINK_LENGTH;

	char* slink = malloc(sizeof(char) * LINK_LENGTH);
	char* linkname = malloc(sizeof(char) * LINK_LENGTH);
	char* str = malloc(sizeof(char) * LINK_LENGTH);
	
	memset(slink, 0, LINK_LENGTH);
	memset(linkname, 0, LINK_LENGTH);
	memset(str, 0, LINK_LENGTH);
	
	DIR* proc_dir = opendir("/proc");

	if (proc_dir == NULL)
	{	
		printf("Error: can not open /proc directory\n");
		return 1;
	}
	
	struct dirent* dir = (struct dirent*)malloc(sizeof(struct dirent)); 
	struct dirent* result;
	long pid;
	int len, is_user = 1, fd;
	FILE* fh;

	while (1)
	{
		readdir_r(proc_dir, dir, &result);

		if (result == NULL)
			break; 

		if (!isdigit(*dir->d_name))
			continue;
		
		pid = strtol(dir->d_name, NULL, 10);
		len = strlen(dir->d_name);
		
		memcpy(slink, "/proc/", strlen("/proc/"));
		memcpy(slink + strlen("/proc/"), dir->d_name, len);
		memcpy(slink + strlen("/proc/") + len, "/exe", strlen("/exe"));
		
		is_user = readlink(slink, linkname, current_size);

		if (strlen(linkname) == current_size)
		{
			linkname = realloc(linkname, current_size * 2);
			current_size *= 2;
			memset(linkname, 0, current_size);
			is_user = readlink(slink, linkname, current_size);

		}

		if (is_user == -1)
		{
			memcpy(slink + strlen("/proc/") + len, "/status", strlen("/status"));				
			
			fd = open(slink, O_RDONLY);
			read(fd, str, LINK_LENGTH);
			
			get_proc(str);
			
			printf("pid: %ld link: %s\n", pid, str);
			
			close(fd);
			
			memset(str, 0, LINK_LENGTH);
			memset(linkname, 0, current_size);
			memset(slink, 0, LINK_LENGTH);			
		
			continue;
		}

		printf("pid: %ld link: \t%s\n", pid, linkname);
		
		memset(linkname, 0, current_size);
		memset(slink, 0, LINK_LENGTH);	
	}	

	closedir(proc_dir);

	free(slink);
	free(linkname);
	free(str);
	free(dir);

	return 0;
}



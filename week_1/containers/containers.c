# define _GNU_SOURCE

#include <stdio.h>
#include <sys/mount.h>
#include <sched.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
 
int main()
{
	int is_parent;

	is_parent = fork();

	if (is_parent == -1)
	{
		printf("Fork failed\n");
		abort();
	}

	int unsh = 0;

	if (is_parent == 0)
	{
		unsh = unshare(CLONE_NEWNS);
		
		if (unsh == -1)
		{
			printf("Unshare failed %s\n", strerror(errno));
			abort();
		}

//		int mk = mkdir("tmp", 0777);	
/*
		if (mk == -1)
		{
			printf("Mkdir failed\n");
			abort();
		}
*/
 		const char* src  = "tmp";
   		const char* trgt = "pt";
   		const char* type = "tmpfs";
   		const unsigned long mntflags = 0;
   		const char* opts = "mode=0700,uid=65534"; 

  		int result = mount(src, trgt, type, mntflags, opts);

		if (result == -1)
		{
			printf("Mount failed: %s\n", strerror(errno));
			abort();
		}
	}

	return 0;

}
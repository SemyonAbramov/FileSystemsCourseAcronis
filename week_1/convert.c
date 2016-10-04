#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

# define	CALC_SYSTEM_BASE	16

typedef enum ret_code
{
	RET_ERR = -1,
	RET_ONE_BYTE = 1,
	RET_TWO_BYTE = 2,
	RET_THREE_BYTE = 3,
	RET_FOUR_BYTE = 4,
	RET_FIVE_BYTE = 5,
	RET_SIX_BYTE = 6				
} ret_code_t;

ret_code_t convert_utf32_to_utf8(uint32_t* utf32_num, uint8_t** utf8_num)			// FIXME: need to implement this function 
{																					// in more proper way 

	int size = 0;
	uint8_t byte1 = 0, byte2 = 0, byte3 = 0, byte4 = 0, byte5 = 0, byte6 = 0;

	if (!((*utf32_num >> 7) | 0x0000))
	{
		size = sizeof(uint8_t);
		*utf8_num = (uint8_t*)malloc(sizeof(uint8_t));
		
		byte1 = *utf32_num & 0x7f;

		memset(*utf8_num, 0, sizeof(char));
		memcpy(*utf8_num, &byte1, size);

		return RET_ONE_BYTE;
	}

	if (!((*utf32_num >> 11) | 0x0000))
	{
		size = sizeof(uint8_t) * 2;
		*utf8_num = (uint8_t*)malloc(size);
		memset(*utf8_num, 0, size);		

		byte1 = 0xc0 | (*utf32_num >> 6);
		byte2 = 0x80 | (*utf32_num & 0x3f);

		memcpy(*utf8_num, &byte1, sizeof(uint8_t));
		memcpy(*utf8_num + sizeof(uint8_t), &byte2, sizeof(uint8_t));	
	
		return RET_TWO_BYTE;
	}

	if (!((*utf32_num >> 16) | 0x0000))
	{
		size = sizeof(uint8_t) * 3;
		*utf8_num = (uint8_t*)malloc(size);
		memset(*utf8_num, 0, size);
		
		byte1 = 0xe0 | ((*utf32_num & 0xf000) >> 12); 
		byte2 = 0x80 | ((*utf32_num & 0xfc0) >> 6);
		byte3 = 0x80 | (*utf32_num & 0x3f);
	
		memcpy(*utf8_num, &byte1, sizeof(uint8_t));
		memcpy(*utf8_num + sizeof(uint8_t), &byte2, sizeof(uint8_t));
		memcpy(*utf8_num + 2 * sizeof(uint8_t), &byte3, sizeof(uint8_t));		
		
		return RET_THREE_BYTE;
	}


	if (!((*utf32_num >> 21) | 0x0000))
	{
		*utf8_num = (uint8_t*)malloc(sizeof(uint8_t) * 4);
			memset(*utf8_num, 0, sizeof(uint8_t) * 4);

		byte1 = 0xf0 | (*utf32_num >> 18);
		byte2 = 0x80 | ((*utf32_num >> 12) & 0x3f);					
		byte3 = 0x80 | ((*utf32_num >> 6) & 0x3f);
		byte4 = 0x80 | (*utf32_num & 0x3f);
	
		memcpy(*utf8_num, &byte1, sizeof(uint8_t));
		memcpy(*utf8_num + sizeof(uint8_t), &byte2, sizeof(uint8_t));
		memcpy(*utf8_num + 2 * sizeof(uint8_t), &byte3, sizeof(uint8_t));				
		memcpy(*utf8_num + 3 * sizeof(uint8_t), &byte4, sizeof(uint8_t));
		
		return RET_FOUR_BYTE;
	}

	if (!((*utf32_num >> 26) | 0x0000))
	{
		*utf8_num = (uint8_t*)malloc(sizeof(uint8_t) * 5);
		memset(*utf8_num, 0, sizeof(uint8_t) * 5);		
	
		byte1 = 0xf8 | (*utf32_num >> 24);
		byte2 = 0x80 | ((*utf32_num >> 18) & 0x3f);
		byte3 = 0x80 | ((*utf32_num >> 12) & 0x3f);
		byte4 = 0x80 | ((*utf32_num >> 6) & 0x3f);
		byte5 = 0x80 | (*utf32_num & 0x3f);		
		
		memcpy(*utf8_num, &byte1, sizeof(uint8_t));
		memcpy(*utf8_num + sizeof(uint8_t), &byte2, sizeof(uint8_t));
		memcpy(*utf8_num + 2 * sizeof(uint8_t), &byte3, sizeof(uint8_t));				
		memcpy(*utf8_num + 3 * sizeof(uint8_t), &byte4, sizeof(uint8_t));	
		memcpy(*utf8_num + 4 * sizeof(uint8_t), &byte5, sizeof(uint8_t));
	
		return RET_FIVE_BYTE;
	}

	if (!((*utf32_num >> 31) | 0x0000))
	{
		*utf8_num = (uint8_t*)malloc(sizeof(uint8_t) * 6);
		memset(*utf8_num, 0, sizeof(char) * 6);		

		byte1 = 0xfc | (*utf32_num >> 30);
		byte2 = 0x80 | ((*utf32_num >> 24) & 0x3f);
		byte3 = 0x80 | ((*utf32_num >> 18) & 0x3f); 
		byte4 = 0x80 | ((*utf32_num >> 12) & 0x3f);
		byte5 = 0x80 | ((*utf32_num >> 6) & 0x3f);
		byte6 = 0x80 | (*utf32_num & 0x3f);

		memcpy(*utf8_num, &byte1, sizeof(uint8_t));
		memcpy(*utf8_num + sizeof(uint8_t), &byte2, sizeof(uint8_t));
		memcpy(*utf8_num + 2 * sizeof(uint8_t), &byte3, sizeof(uint8_t));				
		memcpy(*utf8_num + 3 * sizeof(uint8_t), &byte4, sizeof(uint8_t));	
		memcpy(*utf8_num + 4 * sizeof(uint8_t), &byte5, sizeof(uint8_t));
		memcpy(*utf8_num + 5 * sizeof(uint8_t), &byte6, sizeof(uint8_t));

		return RET_SIX_BYTE;
	}

	return RET_ERR;
}
 
int converter(int inp_fd, int out_fd, char* inp_array)
{
	char* endptr;
	char* cond_p = NULL;
	uint32_t inp_num = 0;
	uint8_t* output;

	endptr = inp_array;

	do {

		cond_p = endptr;
		inp_num = (uint32_t)strtol(endptr, &endptr, CALC_SYSTEM_BASE);

		if (cond_p == endptr)
			break;

		ret_code_t ret = convert_utf32_to_utf8(&inp_num, &output);	
		
		if (ret == RET_ERR)
		{
			printf("Invalid encoding\n");
			abort();
		}

		char* buf = malloc(sizeof(char) * 3);
		int i = 0;

		while (i < ret)
		{
			snprintf(buf, sizeof(char) * 3, "%x", output[i]);

			if (ret != 1)
				write(out_fd, buf, sizeof(char) * 2);
			else
				write(out_fd, buf, sizeof(char) * 1);
			
			i++;
			memset(buf, 0, sizeof(char) * 3);
		}
		
		lseek(out_fd, 1, SEEK_CUR);		
		free(buf);
		free(output);				
	
	} while (1);

	return 0;
}


int main()
{
	int inp_fd = open("conv.txt", O_RDONLY);
	int out_fd = open("output.txt",  O_RDWR | O_CREAT, S_IRWXU);
	
	int fsize = lseek(inp_fd, 0, SEEK_END);
	lseek(inp_fd, 0, SEEK_SET);

	char* inp_array = (char*)malloc(sizeof(char) * fsize);

	read(inp_fd, inp_array, sizeof(char) * fsize);	
	converter(inp_fd, out_fd, inp_array);	

	free(inp_array);
	close(inp_fd);
	close(out_fd);

	return 0;
}


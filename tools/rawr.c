#include <stdlib.h> /* exit func */
#include <stdio.h> /* printf func */
#include <fcntl.h> /* open syscall */
#include <getopt.h> /* args utility */
#include <sys/ioctl.h> /* ioctl syscall*/
#include <unistd.h> /* close syscall */
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>

#include "reader.h"

#define BREADER_PATH "/proc/breader_raw"

int raw_reader(int stride, int times)
{
	int i, read;
	char *raw_buff;
	int64_t sum = 0;
	FILE *file;

	/* 1. Allocate buffer */
	raw_buff = malloc(stride);
	if (!raw_buff)
		return -1;

	/* 2. Open file */
	file = fopen(BREADER_PATH, "r");

	if (file < 0)
		return -1;

	while (times--) {
		/* 3. Read file */
		while (!feof(file)) {
			read = fread(raw_buff, sizeof(char), stride, file);

			for (i = 0; i < read; ++i)
				sum += raw_buff[i];
		}
		rewind(file);
	}


exit:
	/* 4. Close file */
	fclose(file);

	/* 5. Optional result */
	printf("%lu\n", sum);

	return 0;
}
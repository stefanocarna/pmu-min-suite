#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* uintmax_t */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */

#include "reader.h"
#include "../breader/breader_api.h"

#define BREADER_PATH "/proc/breader_mmap"

int mmap_reader(int stride, int times)
{
	int i, j, mmap_size;
	char *raw_buff;
	int64_t sum = 0;
	int fd;

	mmap_size = sysconf(_SC_PAGE_SIZE) << DATA_BUFFER_ORDER;

	/* 1. Open fd */
	fd = open(BREADER_PATH, O_RDWR | O_SYNC);

	if (fd < 0)
		return -1;

	/* 2. Map fd */
	raw_buff = mmap(NULL, mmap_size, PROT_READ, MAP_SHARED, fd, 0);

	if (raw_buff == MAP_FAILED)
		return -1;

	/* 3. Read data */
	while (times--) {
		for (i = 0; i < mmap_size; i += stride)
			for (j = 0; j < stride; ++j)
				sum += raw_buff[j];

		/* Stride is not a multiple of mmap_size */
		for (i; i < mmap_size; ++i)
			sum += raw_buff[j];
	}

	/* 4. Unmap fd */
	if (munmap(raw_buff, mmap_size))
		return -1;

	/* 5. Close fd */
	close(fd);

	printf("%lu\n", sum);

	return 0;
}
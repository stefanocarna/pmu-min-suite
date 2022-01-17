#include <fcntl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* uintmax_t */
#include <string.h>
#include <sys/mman.h>
#include <unistd.h> /* sysconf */

#include "../breader/breader_api.h"

#define BREADER_PATH "/proc/breader_mmap"

int main(int argc, char **argv)
{
	int i;
	int fd;
	long mmap_size;
	uint64_t *buff;
	int64_t sum = 0;

	mmap_size = sysconf(_SC_PAGE_SIZE) << DATA_BUFFER_ORDER;

	int u64_in_mmap = mmap_size >> 3;

	fd = open(BREADER_PATH, O_RDWR | O_SYNC);

	if (fd < 0)
		return fd;

	buff = mmap(NULL, mmap_size, PROT_READ, MAP_SHARED, fd, 0);

	if (buff == MAP_FAILED)
		return -1;

	for (i = 0; i < u64_in_mmap; ++i)
		sum += buff[i];
		// printf("%i: %lx\n", i, buff[i]);

	/* Cleanup. */
	if (munmap(buff, mmap_size))
		return -1;

	printf("%lu\n", sum);

	close(fd);
	return EXIT_SUCCESS;
}

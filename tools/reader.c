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

#define MAX_LEN 256

#define BREADER_PATH "/proc/breader_raw"

int main(int argc, char **argv)
{
	int i, read;
	int mode, times, stride;
	char *endptr;

	if (argc != 4) {
		printf("Usage: %s <mode> <times> <stride>\n", argv[0]);
		return -1;
	}

	mode = strtol(argv[1], &endptr, 0);
	if (errno == ERANGE || *endptr != '\0' || argv[1] == endptr)
		return -1;

	if (mode < 0 || mode > 2) {
		printf("Mode must be in the range [0:2]\n");
		return -1;
	}

	times = strtol(argv[2], &endptr, 0);
	if (errno == ERANGE || *endptr != '\0' || argv[1] == endptr)
		return -1;

	/* Check module 8 */
	if (times < 1) {
		printf("Times must be at least 1\n");
		return -1;
	}

	stride = strtol(argv[3], &endptr, 0);
	if (errno == ERANGE || *endptr != '\0' || argv[1] == endptr)
		return -1;

	/* Check module 8 */
	if (stride & 0x7) {
		printf("Stride must be a multiple of 8\n");
		return -1;
	}

	switch (mode) {
	case 0:
		return seq_reader(stride, times);
	case 1:
		return raw_reader(stride, times);
	case 2:
		return mmap_reader(stride, times);
	default:
		printf("Impossible state\n");
		return -1;
	}
}
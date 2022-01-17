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

#define MAX_LEN 256

#define REGISTER_PATH "/proc/client_module/processes"

int main(int argc, char **argv)
{
	int pid;
	char *exec = argv[1];
	char **exec_args = &argv[1];

	if (argc < 3) {
		printf("Usage: name program [program_args]\n");
		return -1;
	}

	/** Registration phase */
	pid = getpid();
	FILE *file = fopen(REGISTER_PATH, "w");

	if (!file) {
		printf("Cannot open %s. Exiting...\n", REGISTER_PATH);
		exit(-1);
	}

	fprintf(file, "%d", pid);
	fclose(file);

	printf("[WRAPPER] Registered PID %d\n", pid);

	execvp(exec, exec_args);

	return 0;
}
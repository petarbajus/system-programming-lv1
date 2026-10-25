/*
 * U — UNIX low-level copy (open/read/write).
 * Timer: <sys/types.h>, <sys/times.h>, <unistd.h>
 *        hz = sysconf(_SC_CLK_TCK); t = times(NULL);
 *        milliseconds = ((t2 - t1) * 1000) / hz.
 * Override the buffer with -DBUF_SIZE=256|2048|8192.
 */
#ifdef _WIN32
#error Program U is for UNIX. On Windows use UW1, UW2, W1, W2 or W3.
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/times.h>
#include <time.h>

#ifndef BUF_SIZE
#define BUF_SIZE 256
#endif

int main(int argc, char *argv[])
{
	int input_fd, output_fd;
	ssize_t bytes_in, bytes_out;
	char rec[BUF_SIZE];
	clock_t t1, t2, hz;
	struct timespec w1, w2;

	if (argc != 3) {
		printf("Usage: cp file1 file2\n");
		return 1;
	}

	hz = (clock_t)sysconf(_SC_CLK_TCK);
	t1 = times(NULL);
	timespec_get(&w1, TIME_UTC);

	input_fd = open(argv[1], O_RDONLY);
	if (input_fd == -1) {
		perror(argv[1]);
		return 2;
	}
	output_fd = open(argv[2], O_WRONLY | O_CREAT, 0666);
	if (output_fd == -1) {
		perror(argv[2]);
		return 3;
	}

	while ((bytes_in = read(input_fd, &rec, BUF_SIZE)) > 0) {
		bytes_out = write(output_fd, &rec, (size_t)bytes_in);
		if (bytes_out != bytes_in) {
			perror("Fatal write error.");
			return 4;
		}
	}
	close(input_fd);
	close(output_fd);

	t2 = times(NULL);
	timespec_get(&w2, TIME_UTC);
	printf("%ld\n", (long)(((t2 - t1) * 1000) / hz));
	fprintf(stderr, "wall_ms=%lld\n",
		(long long)((w2.tv_sec - w1.tv_sec) * 1000
			+ (w2.tv_nsec - w1.tv_nsec) / 1000000));
	return 0;
}

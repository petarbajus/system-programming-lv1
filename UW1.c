/*
 * UW1 — low-level copy, portable between Windows and UNIX.
 * Windows timer: <time.h>, clock(), result t2 - t1 (CLOCKS_PER_SEC is 1000).
 * UNIX timer:    times(NULL), result ((t2 - t1) * 1000) / hz.
 * Override the buffer with -DBUF_SIZE=256|2048|8192.
 */
#ifdef _WIN32
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/times.h>
#include <time.h>
#endif

#ifndef BUF_SIZE
#define BUF_SIZE 8192
#endif

int main(int argc, char *argv[])
{
	int input_fd, output_fd;
	char rec[BUF_SIZE];
	struct timespec w1, w2;
#ifdef _WIN32
	int bytes_in, bytes_out;
	clock_t t1, t2;
#else
	ssize_t bytes_in, bytes_out;
	clock_t t1, t2, hz;
#endif

	if (argc != 3) {
		printf("Usage: cp file1 file2\n");
		return 1;
	}

#ifdef _WIN32
	t1 = clock();
	timespec_get(&w1, TIME_UTC);

	input_fd = _open(argv[1], O_RDONLY);
	if (input_fd == -1) {
		perror(argv[1]);
		return 2;
	}
	output_fd = _open(argv[2], O_WRONLY | O_CREAT, 0666);
	if (output_fd == -1) {
		perror(argv[2]);
		return 3;
	}

	while ((bytes_in = _read(input_fd, &rec, BUF_SIZE)) > 0) {
		bytes_out = _write(output_fd, &rec, (unsigned int)bytes_in);
		if (bytes_out != bytes_in) {
			perror("Fatal write error.");
			return 4;
		}
	}
	_close(input_fd);
	_close(output_fd);

	t2 = clock();
	timespec_get(&w2, TIME_UTC);
	printf("%ld\n", (long)(t2 - t1));
#else
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
#endif
	fprintf(stderr, "wall_ms=%lld\n",
		(long long)((w2.tv_sec - w1.tv_sec) * 1000
			+ (w2.tv_nsec - w1.tv_nsec) / 1000000));
	return 0;
}

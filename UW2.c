/*
 * UW2 — stdio copy (fopen/fread/fwrite). Portable.
 * Windows timer: clock(), print t2 - t1.
 * UNIX timer:    times(NULL), print ((t2 - t1) * 1000) / hz.
 * Override the buffer with -DBUF_SIZE=256|2048|8192.
 */
#include <stdio.h>
#include <errno.h>
#include <time.h>
#ifndef _WIN32
#include <sys/types.h>
#include <sys/times.h>
#include <unistd.h>
#endif

#ifndef BUF_SIZE
#define BUF_SIZE 256
#endif

int main(int argc, char *argv[])
{
	FILE *in_file, *out_file;
	char rec[BUF_SIZE];
	size_t bytes_in, bytes_out;
	struct timespec w1, w2;
#ifdef _WIN32
	clock_t t1, t2;
#else
	clock_t t1, t2, hz;
#endif

	if (argc != 3) {
		printf("Usage: cp file1 file2\n");
		return 1;
	}

#ifdef _WIN32
	t1 = clock();
#else
	hz = (clock_t)sysconf(_SC_CLK_TCK);
	t1 = times(NULL);
#endif
	timespec_get(&w1, TIME_UTC);

	in_file = fopen(argv[1], "rb");
	if (in_file == NULL) {
		perror(argv[1]);
		return 2;
	}
	out_file = fopen(argv[2], "wb");
	if (out_file == NULL) {
		perror(argv[2]);
		return 3;
	}

	while ((bytes_in = fread(rec, 1, BUF_SIZE, in_file)) > 0) {
		bytes_out = fwrite(rec, 1, bytes_in, out_file);
		if (bytes_out != bytes_in) {
			perror("Fatal write error.");
			return 4;
		}
	}

	fclose(in_file);
	fclose(out_file);

#ifdef _WIN32
	t2 = clock();
	timespec_get(&w2, TIME_UTC);
	printf("%ld\n", (long)(t2 - t1));
#else
	t2 = times(NULL);
	timespec_get(&w2, TIME_UTC);
	printf("%ld\n", (long)(((t2 - t1) * 1000) / hz));
#endif
	fprintf(stderr, "wall_ms=%lld\n",
		(long long)((w2.tv_sec - w1.tv_sec) * 1000
			+ (w2.tv_nsec - w1.tv_nsec) / 1000000));
	return 0;
}

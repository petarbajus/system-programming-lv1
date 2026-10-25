/*
 * W2 — Windows copy with CopyFile. BUF_SIZE is unused: the buffer is inside
 * the system call, so changing it does not change the copy.
 * Timer: <time.h>, clock_t t1, t2; t = clock(); print t2 - t1.
 */
#undef UNICODE
#undef _UNICODE
#undef UNICODE_
#include <windows.h>
#include <stdio.h>
#include <time.h>

#ifndef BUF_SIZE
#define BUF_SIZE 256
#endif

int main(int argc, LPTSTR argv[])
{
	clock_t t1, t2;
	struct timespec w1, w2;

	if (argc != 3) {
		printf("Usage: cp file1 file2\n");
		return 1;
	}

	t1 = clock();
	timespec_get(&w1, TIME_UTC);

	if (!CopyFile(argv[1], argv[2], FALSE)) {
		printf("CopyFile Error: %x\n", GetLastError());
		return 2;
	}

	t2 = clock();
	timespec_get(&w2, TIME_UTC);
	printf("%ld\n", (long)(t2 - t1));
	fprintf(stderr, "wall_ms=%lld\n",
		(long long)((w2.tv_sec - w1.tv_sec) * 1000
			+ (w2.tv_nsec - w1.tv_nsec) / 1000000));
	(void)BUF_SIZE;
	return 0;
}

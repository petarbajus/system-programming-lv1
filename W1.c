/*
 * W1 — Windows copy with ReadFile/WriteFile.
 * Timer: <time.h>, clock_t t1, t2; t = clock(); print t2 - t1.
 * Override the buffer with -DBUF_SIZE=256|2048|8192.
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
	HANDLE hIn, hOut;
	DWORD nIn, nOut;
	CHAR Buffer[BUF_SIZE];
	clock_t t1, t2;
	struct timespec w1, w2;

	if (argc != 3) {
		printf("Usage: cp file1 file2\n");
		return 1;
	}

	t1 = clock();
	timespec_get(&w1, TIME_UTC);

	hIn = CreateFile(argv[1], GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		printf("Cannot open input file. Error: %x\n", GetLastError());
		return 2;
	}

	hOut = CreateFile(argv[2], GENERIC_WRITE, 0, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
		printf("Cannot open output file. Error: %x\n", GetLastError());
		return 3;
	}
	while (ReadFile(hIn, Buffer, BUF_SIZE, &nIn, NULL) && nIn > 0) {
		WriteFile(hOut, Buffer, nIn, &nOut, NULL);
		if (nIn != nOut) {
			printf("Fatal write error: %x\n", GetLastError());
			return 4;
		}
	}
	CloseHandle(hIn);
	CloseHandle(hOut);

	t2 = clock();
	timespec_get(&w2, TIME_UTC);
	printf("%ld\n", (long)(t2 - t1));
	fprintf(stderr, "wall_ms=%lld\n",
		(long long)((w2.tv_sec - w1.tv_sec) * 1000
			+ (w2.tv_nsec - w1.tv_nsec) / 1000000));
	return 0;
}

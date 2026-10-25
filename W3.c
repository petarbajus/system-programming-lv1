/*
 * W3 — Windows copy with sequential scan and a pre-sized output file.
 * Timer: <time.h>, clock_t t1, t2; t = clock(); print t2 - t1.
 * Override the buffer with -DBUF_SIZE=256|2048|8192.
 *
 * GetFileSize writes only the low 32 bits when the high part is NULL, so the
 * preallocation is wrong for files of 4 GB or more (the 5 GB column).
 */
#undef UNICODE
#undef _UNICODE
#undef UNICODE_
#include <windows.h>
#include <stdio.h>
#include <time.h>

#ifndef BUF_SIZE
#define BUF_SIZE 8192
#endif

int main(int argc, LPTSTR argv[])
{
	HANDLE hIn, hOut;
	DWORD nIn, nOut, FsLow;
	CHAR Buffer[BUF_SIZE];
	clock_t t1, t2;
	struct timespec w1, w2;

	if (argc != 3) {
		printf("Usage: cp file1 file2\n");
		return 1;
	}

	t1 = clock();
	timespec_get(&w1, TIME_UTC);

	hIn = CreateFile(argv[1], GENERIC_READ, 0, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hIn == INVALID_HANDLE_VALUE) {
		printf("Cannot open input file. Error: %x\n", GetLastError());
		return 2;
	}
	hOut = CreateFile(argv[2], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hOut == INVALID_HANDLE_VALUE) {
		printf("Cannot open output file. Error: %x\n", GetLastError());
		return 3;
	}

	FsLow = GetFileSize(hIn, NULL);
	SetFilePointer(hOut, FsLow, NULL, FILE_BEGIN);
	SetEndOfFile(hOut);
	SetFilePointer(hOut, 0, NULL, FILE_BEGIN);

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

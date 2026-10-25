/*
 * genfile — create a file of a chosen size.
 *
 * Usage: genfile <output> <size>
 *   size: 1KB, 1MB, 1GB, 5GB, or a plain byte count.
 *
 * The handout generator (fwrite of NBUFS blocks, counter of type long,
 * 1024-byte buffer) is not a large-file program: on Windows long is 32-bit,
 * so a stdio offset cannot name a position past 2 GiB. This one takes the
 * size on the command line and writes it with a 64-bit byte counter. On
 * Windows it uses WriteFile, whose file pointer is 64-bit, so 5 GB works.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifdef _WIN32
#undef UNICODE
#undef _UNICODE
#include <windows.h>
#endif

#define CHUNK (1024u * 1024u)

static int ends_with_ci(const char *s, const char *suffix)
{
	size_t n = strlen(s);
	size_t m = strlen(suffix);
	size_t i;
	if (n < m)
		return 0;
	for (i = 0; i < m; i++) {
		if (tolower((unsigned char)s[n - m + i]) !=
		    tolower((unsigned char)suffix[i]))
			return 0;
	}
	return 1;
}

static unsigned long long parse_size(const char *s)
{
	char *end;
	unsigned long long n;
	const char *unit;

	n = strtoull(s, &end, 10);
	if (end == s)
		return 0;
	while (*end == ' ')
		end++;
	unit = end;
	if (*unit == '\0')
		return n;
	if (ends_with_ci(unit, "KB") || ends_with_ci(unit, "K"))
		return n * 1024ull;
	if (ends_with_ci(unit, "MB") || ends_with_ci(unit, "M"))
		return n * 1024ull * 1024ull;
	if (ends_with_ci(unit, "GB") || ends_with_ci(unit, "G"))
		return n * 1024ull * 1024ull * 1024ull;
	return 0;
}

int main(int argc, char *argv[])
{
	unsigned long long total, left;
	unsigned char *buf;
	size_t i;

	if (argc != 3) {
		fprintf(stderr, "Usage: genfile <output> <size>\n");
		fprintf(stderr, "  size: 1KB, 1MB, 1GB, 5GB, or a byte count\n");
		return 1;
	}

	total = parse_size(argv[2]);
	if (total == 0) {
		fprintf(stderr, "Invalid size: %s\n", argv[2]);
		return 1;
	}

	buf = (unsigned char *)malloc(CHUNK);
	if (buf == NULL) {
		fprintf(stderr, "malloc failed\n");
		return 3;
	}
	srand(1);
	for (i = 0; i < CHUNK; i++)
		buf[i] = (unsigned char)(30 + rand() % 100);

#ifdef _WIN32
	{
		HANDLE h = CreateFileA(argv[1], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (h == INVALID_HANDLE_VALUE) {
			fprintf(stderr, "Cannot create %s. Error: %lx\n", argv[1], GetLastError());
			free(buf);
			return 2;
		}
		left = total;
		while (left > 0) {
			DWORD n = (left > CHUNK) ? CHUNK : (DWORD)left;
			DWORD wrote = 0;
			if (!WriteFile(h, buf, n, &wrote, NULL) || wrote != n) {
				fprintf(stderr, "WriteFile failed. Error: %lx\n", GetLastError());
				CloseHandle(h);
				free(buf);
				return 4;
			}
			left -= wrote;
		}
		CloseHandle(h);
	}
#else
	{
		FILE *f = fopen(argv[1], "wb");
		if (f == NULL) {
			perror(argv[1]);
			free(buf);
			return 2;
		}
		left = total;
		while (left > 0) {
			size_t n = (left > CHUNK) ? CHUNK : (size_t)left;
			if (fwrite(buf, 1, n, f) != n) {
				perror("fwrite");
				fclose(f);
				free(buf);
				return 4;
			}
			left -= n;
		}
		if (fclose(f) != 0) {
			perror("fclose");
			free(buf);
			return 4;
		}
	}
#endif
	free(buf);
	fprintf(stderr, "wrote %llu bytes to %s\n", total, argv[1]);
	return 0;
}

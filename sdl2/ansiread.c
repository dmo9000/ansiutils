#define _POSIX_C_SOURCE	200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include "ansicanvas.h"


extern int errno;

#define CHUNK_SIZE    4096

static char filebuf[4096];

int main(int argc, char *argv[])
{
    FILE *ansfile = NULL;
    char *bufptr = NULL;
    char *endptr = NULL;
    size_t elements_read = 0;
    size_t bytes_read = 0;
    struct stat sbuf;
    size_t total_length = 0;
		off_t offset = 0;
		ANSICanvas *canvas = NULL;


    if (argc < 2) {
        printf("usage: ansiread <filename.ans>\n");
        exit(1);
    }

    lstat(argv[1], &sbuf);
    printf("filesize = %lu\n", sbuf.st_size);
    assert(sbuf.st_size);

		total_length = sbuf.st_size;

    ansfile = fopen(argv[1], "rb");

    if (!ansfile) {
        printf("cannot open: %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }

		canvas = new_canvas();

    while (!feof(ansfile) && !ferror(ansfile) && offset < total_length) {
    		printf("offset: %lu, total_length: %lu\n", offset, total_length);
        bufptr = (char *) &filebuf;
        elements_read = fread((char *) bufptr, CHUNK_SIZE, 1, ansfile);
        bytes_read = (elements_read * CHUNK_SIZE);
        printf("[%ld] bytes read to buffer\n", bytes_read);
        endptr = (char *) bufptr + (bytes_read);
        printf("\n");
        printf("[reached end of buffer, offset = %lu, bytes_read=%lu]\n", offset, bytes_read);
    }
    fclose(ansfile);
    assert(!endptr);
    exit (0);
}


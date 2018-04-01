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
#include "rawfont.h"
#include "gfx.h"


extern int errno;

#define CHUNK_SIZE    4096

static unsigned char filebuf[CHUNK_SIZE];

bool ansi_to_canvas(ANSICanvas *c, unsigned char *buf, size_t nbytes);
BitmapFont *bmf_load(char *filename);
/*
extern int gfx_main(uint16_t, uint16_t);
extern int gfx_drawglyph(BitmapFont *bmf, uint8_t px, uint8_t py, uint8_t glyph);
extern int gfx_expose();
*/

int main(int argc, char *argv[])
{
    FILE *ansfile = NULL;
    size_t bytes_read = 0;
    struct stat sbuf;
    size_t total_length = 0;
    off_t offset = 0;
    ANSICanvas *canvas = NULL;
    uint16_t width = 0, height = 0;
    BitmapFont *myfont = NULL;
    char *font_filename = NULL;


    if (argc < 2) {
        printf("usage: ansiread <filename.ans>\n");
        exit(1);
    }

    font_filename = "bmf/8x8.bmf";

    myfont = bmf_load(font_filename);
    if (!myfont) {
        perror("bmf_load");
        exit(1);
    }

    //gfx_main((CANVAS_WIDTH*8), (CANVAS_HEIGHT*16));


    lstat(argv[1], &sbuf);
    printf("filesize = %lu\n", sbuf.st_size);
    assert(sbuf.st_size);

    total_length = sbuf.st_size;

    ansfile = fopen(argv[1], "rb");

    if (!ansfile) {
        printf("cannot open: %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }


    /* create the canvas */

    canvas = new_canvas();
    assert(canvas);

    while (!feof(ansfile) && !ferror(ansfile) && offset < total_length) {
        bytes_read = fread((unsigned char *) &filebuf, 1, CHUNK_SIZE, ansfile);
        ansi_to_canvas(canvas, (unsigned char *) &filebuf, bytes_read);
        offset+=bytes_read;
    }
    printf("[%ld] total bytes processed\n", offset);
    fclose(ansfile);
    width = canvas_get_width(canvas);
    height = canvas_get_height(canvas);


    printf("canvas dimensions: %u x %u\n", width, height);

    canvas_output(canvas, false, false, NULL);

    gfx_main((width*8), (height*16));

    gfx_canvas_render(canvas, myfont);

    gfx_expose();

    while (!getchar()) {
    }


    exit (0);
}


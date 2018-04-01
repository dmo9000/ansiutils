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
extern bool auto_line_wrap;

#define CHUNK_SIZE    4096

static unsigned char filebuf[CHUNK_SIZE];

bool ansi_to_canvas(ANSICanvas *c, unsigned char *buf, size_t nbytes);
void raster_extend_length_to(ANSIRaster *r, uint16_t extrabytes);

BitmapFont *bmf_load(char *filename);

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
    int8_t c = 0;
    char *input_filename = NULL;


    if (argc < 2) {
        printf("usage: ansiread <filename.ans>\n");
        exit(1);
    }

     while ((c = getopt (argc, argv, "w")) != -1) {
        switch (c)
        {
        case 'w':
            /* append sauce record */
            printf("WRAP MODE ENABLED\n");
            auto_line_wrap = true;
            break;
        case -1:
            /* END OF ARGUMENTS? */
            break;
        default:
            printf("exiting with c= %d [%c]\n", c, c);
            exit (1);
        }

    }

    font_filename = "bmf/8x8.bmf";

    myfont = bmf_load(font_filename);
    if (!myfont) {
        perror("bmf_load");
        exit(1);
    }

    input_filename = (char *) argv[optind];


    lstat(input_filename, &sbuf);
    printf("filesize = %lu\n", sbuf.st_size);
    assert(sbuf.st_size);
    total_length = sbuf.st_size;
    

    ansfile = fopen(input_filename, "rb");

    if (!ansfile) {
        printf("cannot open: %s: %s\n", input_filename, strerror(errno));
        exit(1);
    }

    /* create the canvas */

    canvas = new_canvas();
    assert(canvas);

    while (!feof(ansfile) && !ferror(ansfile) && offset < total_length) {
        bytes_read = fread((unsigned char *) &filebuf, 1, CHUNK_SIZE, ansfile);
        if (!ansi_to_canvas(canvas, (unsigned char *) &filebuf, bytes_read)) {
            break;
        };
        offset+=bytes_read;
    }
    printf("[%ld] total bytes processed\n", offset);
    fclose(ansfile);


    if (auto_line_wrap) {
        /* pad lines */
        for (int i = 0; i < canvas_get_height(canvas); i++) {
            ANSIRaster *r = canvas_get_raster(canvas, i);
            if (r->bytes < 80) {
                raster_extend_length_to(r, 80);
                }
            }
        }

    width = canvas_get_width(canvas);
    height = canvas_get_height(canvas);


    printf("canvas dimensions: %u x %u\n", width, height);

    canvas_output(canvas, true,  NULL);

    gfx_main((width*8), (height*16), input_filename);

    gfx_canvas_render(canvas, myfont);

    gfx_expose();

    while (!getchar()) {
    }


    exit (0);
}


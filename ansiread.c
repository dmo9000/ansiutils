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
#include <getopt.h>
#include <pthread.h>
#include "ansicanvas.h"
#include "rawfont.h"
#include "gfx.h"


/* Prototype missing from c99 */
char *strdup(const char *s);

//extern int errno;
extern bool auto_line_wrap;
extern bool allow_clear;
bool auto_line_padding = false;
extern bool debug_flag;

#define CHUNK_SIZE    4096

static unsigned char filebuf[CHUNK_SIZE];

bool ansi_to_canvas(ANSICanvas *c, unsigned char *buf, size_t nbytes, size_t offset);

BitmapFont *bmf_load(char *filename);

pthread_t graphics_thread;

uint16_t gfx_opengl_width = 0;
uint16_t gfx_opengl_height = 0;


void rungraphics()
{

    printf("rungraphics()\r\n");
    fflush(NULL);
    gfx_opengl_main(gfx_opengl_width, gfx_opengl_height, "MyAmazingWindowTitle");
    while (1) { }
}

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
    char *output_filename = NULL;
    char *png_filename = NULL;
    bool graphic_preview = false;
    bool text_output = true;
    bool enable_utf8 = true;
    bool enable_compression = false;

		printf("ansiread starting ...\n");
    if (argc < 2) {
        printf("usage: ansiread <filename.ans>\n");
        exit(1);
    }

    while ((c = getopt (argc, argv, "wf:o:rgczp")) != -1) {
        switch (c)
        {
        case 'c':
            printf("UTF8 DISABLED - USING CP437 OUTPUT\n");
            enable_utf8 = false;
            break;
        case 'f':
            /* send output to file instead of terminal */
            assert(optarg);
            output_filename = strdup(optarg);
            printf("+++ OUTPUT FILENAME SET TO %s\n", output_filename);
            break;
        case 'g':
            /* enable graphic preview */
            graphic_preview = true;
            text_output = false;
            break;
#ifndef __MINGW__
        case 'o':
            png_filename = strdup(optarg);
            text_output = false;
            break;
#endif
        case 'p':
            /* enable line padding */
            auto_line_padding = true;
            break;
        case 'r':
            printf("ALLOW CLEAR MODE SET\n");
            allow_clear = true;
            break;
        case 'w':
            /* append sauce record */
            printf("WRAP MODE ENABLED\n");
            auto_line_wrap = true;
            break;
        case 'z':
            enable_compression = true;
            break;
        case -1:
            /* END OF ARGUMENTS? */
            break;
        default:
            printf("exiting with c= %d [%c]\n", c, c);
            exit (1);
        }

    }

    input_filename = (char *) argv[optind];

#ifndef __MINGW__
    lstat(input_filename, &sbuf);
#else
    stat(input_filename, &sbuf);
#endif /* __MINGW__*/ 
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

    if (enable_compression) {
        canvas->compress_output = true;
    }

    while (!feof(ansfile) && !ferror(ansfile) && offset < total_length) {
        bytes_read = fread((unsigned char *) &filebuf, 1, CHUNK_SIZE, ansfile);
        if (!ansi_to_canvas(canvas, (unsigned char *) &filebuf, bytes_read, offset)) {
            break;
        };
        offset+=bytes_read;
    }
    printf("[%ld] total bytes processed\n", offset);
    fclose(ansfile);

    if (auto_line_padding) {
        printf("\n");
        /* pad lines */
        for (int i = 0; i < canvas->lines; i++) {
            if (debug_flag) {
                printf("\rPadding line %u/%u", i, canvas->lines);
            }
            ANSIRaster *r = canvas_get_raster(canvas, i);
            if (r->bytes > 80) {
                if (debug_flag) {
                    printf("\r\nTrimming line %u/%u\n", i, canvas->lines);
                }
                r->bytes = 80;
            }
            if (r->bytes < 80) {
                raster_extend_length_to(r, 80);
            }
        }
        printf("\n");
    }

    width = canvas_get_width(canvas);
    height = canvas_get_height(canvas);


    printf("canvas dimensions: %u x %u\n", width, height);

    if (text_output) {
        if (output_filename) {
            printf("Rendering to file, with compression %s\n", (enable_compression ? "enabled" : "disabled"));
        } else {
            printf("Rendering to tty, with compression %s\n", (enable_compression ? "enabled" : "disabled"));
        }
        canvas_output(canvas, enable_utf8,output_filename);
    }

    if (graphic_preview || png_filename) {
        font_filename = "bmf/8x8.bmf";
        myfont = bmf_load(font_filename);
        if (!myfont) {
            perror("bmf_load");
            exit(1);
        }
    }


    if (graphic_preview) {
        printf("Rendering OpenGL preview ...\n");
        //gfx_sdl_main((width*8), (height*16), input_filename);


				gfx_opengl_width = (width*8);
				gfx_opengl_height = (height*16);

				pthread_create( &graphics_thread, NULL, rungraphics, NULL);

				sleep(1);

        gfx_opengl_canvas_render(canvas, myfont);
        gfx_opengl_expose();

        printf("Hit ENTER to close preview.\n");
        //while (!getchar()) { }
				while (1) { } 
    }

#ifndef __MINGW__
    if (png_filename) {
        printf("Rendering PNG output to %s ...\n", png_filename);
        gfx_png_main((width*8), (height*16));
        gfx_png_canvas_render(canvas, myfont);
        gfx_png_export(png_filename);
    }
#endif

    exit (0);
}


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include "ansicanvas.h"
#include "rawfont.h"
#include "gfx_opengl.h"
#include "tdf.h"
#include "bmf.h"
#include "tdffont.h"
#include "ansiraster.h"
#include "ansicanvas.h"
#include "8x8.h"

#define CANVAS_WIDTH    80
#define CANVAS_HEIGHT   24

ANSICanvas *my_canvas = NULL;
BitmapFont *bmf_load(char *filename);

extern int ansi_read(char *ansi_file_name);
//extern int gfx_opengl_drawglyph(BitmapFont *font, uint8_t px, uint8_t py, uint8_t glyph, uint8_t fg, uint8_t bg, uint8_t attr);
extern int gfx_opengl_expose();

pthread_t graphics_thread;


void *rungraphics(void * v)
{

    printf("rungraphics()\r\n");
    fflush(NULL);
    //gfx_opengl_main(640, 384, "rawfont viewer");
    gfx_opengl_main(my_canvas, gfx_opengl_getwidth(), gfx_opengl_getheight(), 1, "rawfont preview");
    while (1) {
        sleep(1);
    }
}

int main(int argc, char *argv[])
{
//   DEPRECATED/REMOVE: char *filename = (char *) argv[1];
    BitmapFont *myfont;
//    filename = "bmf/8x8.bmf";

    myfont = bmf_embedded(bmf_8x8_bmf);

    if (!myfont) {
        perror("bmf_load");
        exit(1);
    }

    my_canvas = new_canvas();

    canvas_setdimensions(my_canvas, 80, 24);

    gfx_opengl_setdimensions(640, 384);
    pthread_create( &graphics_thread, NULL, rungraphics, NULL);

    sleep(1);

    for (int kk = 0; kk < 256; kk++) {
        gfx_opengl_drawglyph(myfont, (kk % CANVAS_WIDTH), (kk / CANVAS_WIDTH), kk, 7, 0, ATTRIB_NONE);
    }
//    ansi_read("ansifiles/fruit.ans");

    gfx_opengl_expose();
    while (!getchar()) {
        sleep(1);
    }
    exit(0);

}


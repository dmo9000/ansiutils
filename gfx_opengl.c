#include <GL/glut.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include "rawfont.h"
#include "ansicanvas.h"
#include "gfx_opengl.h"

/* set this if you want to skip frames */
#define FRAME_SKIP

/* Display size */
#define SCREEN_WIDTH    640
#define SCREEN_HEIGHT   384

int display_width = 0;
int display_height = 0;
int modifier = 2;

extern int g_trace;

typedef unsigned char u8;
u8 *screenData = NULL;
int gfx_opengl_drawglyph(BitmapFont *font, uint16_t px, uint16_t py, uint8_t glyph, uint8_t fg, uint8_t bg, uint8_t attr);
extern BitmapFont *myfont;

#define MAX_KBBUF_LEN	256
unsigned char kbbuf[MAX_KBBUF_LEN];
volatile uint8_t kbbuf_len = 0;

uint16_t gfx_opengl_width;
uint16_t gfx_opengl_height;
static bool glut_initialised = false;

ANSICanvas *myCanvas = NULL;

pthread_mutex_t gfx_mutex;

uint16_t gfx_opengl_getwidth()
{
    return gfx_opengl_width;
}

uint16_t gfx_opengl_getheight()
{
    return gfx_opengl_height;
}


void gfx_opengl_setdimensions(uint16_t w, uint16_t h)
{

    printf("gfx_opengl_setdimenions(%u, %u)\n", w, h);
    gfx_opengl_width = w;
    gfx_opengl_height = h;

}

void updateTexture()
{
    if (!myCanvas) return;

    if (canvas_is_dirty(myCanvas)) {
        //printf("updateTexture() dirty\n");
    } else {
        //   printf("updateTexture() clean\n");
        usleep(16666);
        //pthread_yield();
        return;
    }

    while (pthread_mutex_trylock(&gfx_mutex) != 0) {
        /* try frame skip */
#ifdef FRAME_SKIP
        usleep(16666);
        return;
#else
        pthread_yield();
#endif
    }


    glTexSubImage2D(GL_TEXTURE_2D, 0,0, 0, gfx_opengl_width, gfx_opengl_height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

    glBegin( GL_QUADS );
    glTexCoord2d(0.0, 0.0);
    glVertex2d(0.0, 0.0);
    glTexCoord2d(1.0, 0.0);
    glVertex2d(display_width, 0.0);
    glTexCoord2d(1.0, 1.0);
    glVertex2d(display_width, display_height);
    glTexCoord2d(0.0, 1.0);
    glVertex2d(0.0,  display_height);
    glEnd();

    if (glut_initialised) {
        glutSwapBuffers();
    }

    myCanvas->is_dirty=false;
    pthread_mutex_unlock(&gfx_mutex);
    usleep(16666);
}


void display()
{
    //glClear(GL_COLOR_BUFFER_BIT);
    updateTexture();
//    if (glut_initialised) {
//        glutSwapBuffers();
//    }
}

void reshape_window(GLsizei w, GLsizei h)
{
    //printf("reshape_window(w=%u,h=%u)\r\n", w, h);
    glClearColor(0.0f, 0.0f, 0.5f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, h, 0);
    glMatrixMode(GL_MODELVIEW);
    glViewport(0, 0, w, h);
    display_width = w;
    display_height = h;
    myCanvas->is_dirty= true;
}


// Setup Texture
void setupTexture()
{

    //printf("setupTexture(%ux%u)\r\n", gfx_opengl_width, gfx_opengl_height);
    screenData = malloc(gfx_opengl_width * gfx_opengl_height * 3);

    // Create a texture
    glTexImage2D(GL_TEXTURE_2D, 0, 3, gfx_opengl_width, gfx_opengl_height, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);

    // Set up the texture
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // Enable textures
    glEnable(GL_TEXTURE_2D);
}


void setTexturePixel(int x, int y, u8 r, u8 g, u8 b)
{
    unsigned char *scrP;

    /* FIXME: busy wait until surface becomes available */

    if (!screenData) return;

    scrP = screenData;

    scrP += (y * gfx_opengl_width * 3) + (x * 3);
    *scrP = r;
    scrP++;
    *scrP = g;
    scrP++;
    *scrP = b;

}


int gfx_opengl_expose()
{
//  printf("gfx_opengl_expose()\n");
    assert(myCanvas);
    while (pthread_mutex_trylock(&gfx_mutex) != 0) {
        //usleep(10000);
        pthread_yield();
    }

    myCanvas->is_dirty = true;
    pthread_mutex_unlock(&gfx_mutex);

    return 0;
}

int gfx_opengl_hwscroll()
{

    int x =0, y = 0;
    char *src_addr, *dest_addr = NULL;
//    printf("gfx_opengl_hwscroll()\r\n");

    src_addr = screenData;
    dest_addr = screenData;
    src_addr += (y * gfx_opengl_width * 3) + (x * 3);
    dest_addr += ((y+16) * gfx_opengl_width * 3) + (x * 3);

    for (int y = 0; y < gfx_opengl_height - 16; y++) {
        //for(int x = 0; x < gfx_opengl_width; x++) {
        src_addr = screenData;
        dest_addr = screenData;
        src_addr += ((y+16) * gfx_opengl_width * 3); //+ (x * 3);
        dest_addr += (y * gfx_opengl_width * 3); // + (x * 3);
        memcpy(dest_addr, src_addr, gfx_opengl_width *3);
        //}
    }

    for(int y = (gfx_opengl_height-16); y < (gfx_opengl_height); y++)  {
        dest_addr = screenData;
        dest_addr += (y * gfx_opengl_width * 3); // + (x * 3);
        memset(dest_addr, 0, gfx_opengl_width *3);
    }

//		myCanvas->is_dirty=true;
    return 1;
}

int gfx_opengl_drawglyph(BitmapFont *font, uint16_t px, uint16_t py, uint8_t glyph, uint8_t fg, uint8_t bg, uint8_t attr)
{

    RGBColour *fgc;
    RGBColour *bgc;
    uint8_t rx = 0;
    uint8_t h = 0;

    /*
    if (py >=24 ) {
    	printf("drawglyph(line %u) out of bounds\r\n", py);
    	assert(NULL);
    	return 0;
    }
    */

    //printf("gfx_opengl_drawglyph(%u, %u, %u, %u, '%c', fg=%u, bg=%u)\n", px, py, font->header.px, font->header.py, glyph, fg, bg);

    while (pthread_mutex_trylock(&gfx_mutex) != 0) {
        //usleep(10000);
        pthread_yield();
    }

    if (attr & ATTRIB_REVERSE) {
        bgc = canvas_displaycolour(fg + ((attr & ATTRIB_BOLD ? 8 : 0)));
        fgc = canvas_displaycolour(bg);
    } else {
        fgc = canvas_displaycolour(fg + ((attr & ATTRIB_BOLD ? 8 : 0)));
        bgc = canvas_displaycolour(bg);
    }

		assert(font->fontdata);
    for (uint8_t ii = 0; ii < font->header.py; ii++) {
        h = 0;
        for (uint8_t jj = 128; jj >0; jj = jj >> 1) {
            //printf("%u -> %u, ", r, jj);
            rx = font->fontdata[(glyph*font->header.py) + ii];

            if (rx & jj || ((attr & ATTRIB_UNDERLINE) && (ii == font->header.py - 1))) {
                setTexturePixel((px*8) + h, (py*16)+(ii*2), fgc->r, fgc->g, fgc->b);
                setTexturePixel((px*8) + h, (py*16)+(ii*2)+1, fgc->r, fgc->g, fgc->b);
//                printf("X");
            } else {
                setTexturePixel((px*8) + h, (py*16)+(ii*2), bgc->r, bgc->g, bgc->b);
                setTexturePixel((px*8) + h, (py*16)+(ii*2)+1, bgc->r, bgc->g, bgc->b);
//                printf(" ");
            }
            h++;
        }
        //  printf("\n");
    }

    pthread_mutex_unlock(&gfx_mutex);

    return 0;
}

int input_character()
{

    uint8_t ch = 0;
    int i = 0;
    while (kbbuf_len == 0) {
    }
    ch= kbbuf[0];
    for (i = 0; i < kbbuf_len-1; i++) {
        kbbuf[i] = kbbuf[i+1];
    }
    kbbuf_len --;
    return ch;

}


void process_Normal_Keys(int key, int x, int y)
{
    //printf("process_Normal_Keys()\r\n");

    switch (key)
    {
    default:
        //printf("GLUT_KEY(%d)\r\n", key);
        //output_character(key);
        assert(kbbuf_len < MAX_KBBUF_LEN) ;
        kbbuf[kbbuf_len] = key;
        kbbuf_len++;
        //printf("keyboard buffer len is now %u\r", kbbuf_len);
        break;
    }

}


int gfx_opengl_main(ANSICanvas *c, uint16_t xsize, uint16_t ysize, int multiplier, char *WindowTitle)
{
    int argc = 0;
    char *argv[] = { NULL };
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);

    display_width = xsize;
    display_height = ysize;

    glutInitWindowSize(display_width*multiplier, display_height*multiplier);
//    glutInitWindowPosition(320, 320);
    glutCreateWindow(WindowTitle);

    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutReshapeFunc(reshape_window);
    glutKeyboardFunc( process_Normal_Keys );

    assert(c);
    myCanvas = c;

    setupTexture();

    glut_initialised = true;

    glutMainLoop();

    return 0;
}

void  gfx_opengl_clear()
{
    //OGL_RenderClear( renderer);

}

int gfx_opengl_canvas_render(ANSICanvas *canvas, BitmapFont *myfont)
{
    ANSIRaster *r = NULL;
    uint16_t width = 0, height = 0;
    assert(canvas);
    width = canvas_get_width(canvas);
    height = canvas_get_height(canvas);

    assert(width);
    assert(height);

//    printf("gfx_opengl_canvas_render(%ux%u, %u)\r\n", width, height, canvas->lines);
    for (uint16_t ii = 0; ii < height; ii++) {
        r = canvas_get_raster(canvas, ii);
        if (r) {
            for (uint16_t jj = 0; jj < r->bytes; jj++) {
                /* FIXME: call gfx_opengl_canvas_render_xy() instead */
                if (ii < canvas->lines && (canvas->scroll_limit ? (ii < canvas->scroll_limit) : ii < canvas->lines)) {
                    gfx_opengl_drawglyph(myfont, jj, ii, r->chardata[jj], r->fgcolors[jj], r->bgcolors[jj], r->attribs[jj]);
                }
            }
        } else {
            printf("canvas data missing for raster %u\n", ii);
        }
    }
    return 0;
}

int gfx_opengl_canvas_render_xy(ANSICanvas *canvas, BitmapFont *myfont, uint16_t x, uint16_t y)
{
    ANSIRaster *r = NULL;

//		fprintf(stderr, "gfx_opengl_canvas_render_xy(..., %u, %u)\r\n", x, y);
    if (y > canvas->scroll_limit) {
        //printf("y > 24 (=%u) ; canvas->scroll_limit = %u\r\n", y, canvas->scroll_limit);
        return 0;
    }

    r = canvas_get_raster(canvas, y);
    if (!r) {
        printf("canvas_get_raster(%u) failed\n", y);
    }
    assert(r);
    if (!r->chardata) {
        printf("+++ gfx_opengl_canvas_render_xy(%u,%u) -> failed\n", x, y);
        assert(r->chardata);
    }
    //assert(x < r->bytes);
    if (x < r->bytes) {
        gfx_opengl_drawglyph(myfont, x, y, r->chardata[x], r->fgcolors[x], r->bgcolors[x], r->attribs[x]);
    }
    return 1;
}

int gfx_opengl_render_cursor(ANSICanvas *canvas, BitmapFont *myfont, uint16_t x,  uint16_t y, bool state)
{
    ANSIRaster *r = NULL;

    assert(canvas);

    //printf("gfx_opengl_render_cursor(0x%lx, 0x%lx, %u, %u, %s)\r\n", canvas, myfont, x, y, (state ? "true" : "false"));

    if (y >=24) {
        return 0;
    }
    assert(y < 24);

    r = canvas_get_raster(canvas, y);
    if (!r) {
        printf("canvas_get_raster(%u) failed\n", y);
    }
    assert(r);
    if (!r->chardata) {
        printf("+++ gfx_opengl_canvas_render_xy(%u,%u) -> failed\n", x, y);
        assert(r->chardata);
    }


    switch (state) {
    case true:
        if (canvas->cursor_enabled) {
            gfx_opengl_drawglyph(myfont, x, y, ' ', 0, 7, ATTRIB_NONE);
        }
        break;
    case false:
        gfx_opengl_drawglyph(myfont, x, y, r->chardata[x], r->fgcolors[x], r->bgcolors[x], r->attribs[x]);
        break;
    }

    canvas->is_dirty=true;

    return 1;
}

/* compat functions for z80p - need to refactor */

int tty_getbuflen()
{
    return kbbuf_len;

}

int tty_popkeybuf()
{
    return input_character();
}

int tty_processinput()
{
    if (!kbbuf_len) {
        return 0;
    }
    return 1;
}



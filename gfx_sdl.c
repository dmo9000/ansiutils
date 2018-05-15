#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "rawfont.h"
#include "ansicanvas.h"

SDL_Window* window;
SDL_Renderer* renderer;

int gfx_sdl_expose()
{

    SDL_RenderPresent(renderer);
    return 0;
}

int gfx_sdl_drawglyph(BitmapFont *font, uint16_t px, uint16_t py, uint8_t glyph, uint8_t fg, uint8_t bg, uint8_t attr)
{
    RGBColour *fgc;
    RGBColour *bgc;
    uint8_t rx = 0;
    uint8_t h = 0;
    SDL_Rect r;
    //printf("gfx_sdl_drawglyph(%u, %u, %u, %u, '%c')\n", px, py, font->header.px, font->header.py, glyph);
    //
    fgc = canvas_displaycolour(fg + ((attr & ATTRIB_BOLD ? 8 : 0)));
    bgc = canvas_displaycolour(bg);


    for (int ii = 0; ii < font->header.py; ii++) {
        h = 0;
        /* TODO: handle big-endian */
        for (int jj = 128; jj >0; jj = jj >> 1) {

            r.x = (px*8) + (h*1);
            r.y = (py*16) + (ii*2);
            r.w = 1;
            r.h = 2;

            //printf("%u -> %u, ", r, jj);
            rx = font->fontdata[(glyph*font->header.py) + ii];
            if (rx & jj) {



                SDL_SetRenderDrawColor( renderer, fgc->r, fgc->g, fgc->b, 255 );
                SDL_RenderFillRect( renderer, &r );
                //SDL_RenderDrawPoint(renderer, (px*16) + (h*2), (py*16) + (ii*2));
                //printf("X");
            } else {
                SDL_SetRenderDrawColor( renderer, bgc->r, bgc->g, bgc->b, 255 );
                SDL_RenderFillRect( renderer, &r );
                //SDL_RenderDrawPoint(renderer, (px*16) + (h*2), (py*16) + (ii*2));
                //printf(" ");
            }
            h++;
        }
        //printf("\n");
    }
//    SDL_RenderPresent(renderer);
    return 0;
}

int gfx_sdl_main(uint16_t xsize, uint16_t ysize, char *WindowTitle)
{
    int posX = 100;
    int posY = 200;
    int sizeX = xsize;
    int sizeY =  ysize;

    if ( SDL_Init( SDL_INIT_EVERYTHING ) != 0 )
    {
        // Something failed, print error and exit.
        printf(" Failed to initialize SDL : %s\n", SDL_GetError());
        return -1;
    }

    window = SDL_CreateWindow(WindowTitle, posX, posY, sizeX, sizeY, 0 );

    if ( window == NULL )
    {
        printf( "Failed to create window : %s", SDL_GetError());
        return -1;
    }

    renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );

    if ( renderer == NULL )
    {
        printf( "Failed to create renderer : %s", SDL_GetError());
        return -1;
    }

    SDL_RenderSetLogicalSize( renderer, sizeX, sizeY );
    SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
    SDL_RenderClear( renderer );
    SDL_RenderPresent( renderer);
    return 0;
}

void  gfx_sdl_clear()
{
    SDL_RenderClear( renderer);

}

int gfx_sdl_canvas_render(ANSICanvas *canvas, BitmapFont *myfont)
{
    ANSIRaster *r = NULL;
    uint16_t width = 0, height = 0;
    assert(canvas);
    width = canvas_get_width(canvas);
    height = canvas_get_height(canvas);

    assert(width);
    assert(height);

    //printf("gfx_sdl_canvas_render(%ux%u)\n", width, height);
    for (uint16_t ii = 0; ii < height; ii++) {
        r = canvas_get_raster(canvas, ii);
        if (r) {
            for (uint16_t jj = 0; jj < r->bytes; jj++) {
                /* FIXME: call gfx_sdl_canvas_render_xy() instead */
                gfx_sdl_drawglyph(myfont, jj, ii, r->chardata[jj], r->fgcolors[jj], r->bgcolors[jj], r->attribs[jj]);
            }
        } else {
            printf("canvas data missing for raster %u\n", ii);
        }
    }
    return 0;
}

int gfx_sdl_canvas_render_xy(ANSICanvas *canvas, BitmapFont *myfont, uint16_t x, uint16_t y)
{
    ANSIRaster *r = NULL;

    assert(y <= 24);
    r = canvas_get_raster(canvas, y);
    if (!r) {
        printf("canvas_get_raster(%u) failed\n", y);
    }
    assert(r);
    if (!r->chardata) {
        printf("+++ gfx_sdl_canvas_render_xy(%u,%u) -> failed\n", x, y);
        assert(r->chardata);
        }
    //assert(x < r->bytes);
    if (x < r->bytes) {
        gfx_sdl_drawglyph(myfont, x, y, r->chardata[x], r->fgcolors[x], r->bgcolors[x], r->attribs[x]);
    }
    return 1;
}

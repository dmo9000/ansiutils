#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdint.h>
#include "rawfont.h"

SDL_Window* window;
SDL_Renderer* renderer;

int gfx_expose()
{

    SDL_RenderPresent(renderer);
    return 0;
}

int gfx_drawglyph(BitmapFont *font, uint8_t px, uint8_t py, uint8_t glyph)
{
    uint8_t rx = 0;
    uint8_t h = 0;
    SDL_Rect r;
    //printf("gfx_drawglyph(%u, %u, %u, %u, '%c')\n", px, py, font->header.px, font->header.py, glyph);

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
                SDL_SetRenderDrawColor( renderer, 255, 255, 255, 255 );
                SDL_RenderFillRect( renderer, &r );
                //SDL_RenderDrawPoint(renderer, (px*16) + (h*2), (py*16) + (ii*2));
                //printf("X");
            } else {
                SDL_SetRenderDrawColor( renderer, 0,0,0, 255 );
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

int gfx_main(uint16_t xsize, uint16_t ysize)
{
    int posX = 100;
    int posY = 200;
    int sizeX = xsize;
    int sizeY =  ysize;

    // Initialize SDL
    // ==========================================================
    if ( SDL_Init( SDL_INIT_EVERYTHING ) != 0 )
    {
        // Something failed, print error and exit.
        printf(" Failed to initialize SDL : %s\n", SDL_GetError());
        return -1;
    }

    // Create and init the window
    // ==========================================================
    window = SDL_CreateWindow( "Test", posX, posY, sizeX, sizeY, 0 );

    if ( window == NULL )
    {
        printf( "Failed to create window : %s", SDL_GetError());
        return -1;
    }

    // Create and init the renderer
    // ==========================================================
    renderer = SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );

    if ( renderer == NULL )
    {
        printf( "Failed to create renderer : %s", SDL_GetError());
        return -1;
    }

    // Render something
    // ==========================================================

    // Set size of renderer to the same as window
    SDL_RenderSetLogicalSize( renderer, sizeX, sizeY );

    // Set color of renderer to red
    SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );

    // Clear the window and make it all red
    SDL_RenderClear( renderer );

    // Render the changes above ( which up until now had just happened behind the scenes )
    SDL_RenderPresent( renderer);

    // Pause program so that the window doesn't disappear at once.
    // This willpause for 4000 milliseconds which is the same as 4 seconds
    //SDL_Delay( 4000 );
    return 0;
}

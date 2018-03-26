#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdint.h>

SDL_Window* window;
SDL_Renderer* renderer;

int gfx_drawglyph(uint8_t *fontdata, uint8_t height, uint16_t px, uint16_t py, uint16_t glyph)
{
		uint8_t r = 0;
		printf("gfx_drawglyph(%u, %u, %u, '%c')\n", height, px, py, glyph);

		for (int ii = 0; ii < height; ii++) {
				for (int jj = 1; jj < 256; jj = jj << 2) {
					printf("%u -> %u, ", r, jj);
					r = fontdata[(glyph*height) + ii];	
					if (r & jj) {
							//printf("X");
							} else {
							//printf(" ");
							}	
					}
					printf("\n");
				}
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
    window = SDL_CreateWindow( "Server", posX, posY, sizeX, sizeY, 0 );

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

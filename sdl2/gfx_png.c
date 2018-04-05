#include <SDL2/SDL.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <png.h>
#include "rawfont.h"
#include "ansicanvas.h"

/* see http://www.labbookpages.co.uk/software/imgProc/libPNG.html */

FILE *fp = NULL;
png_structp png;
png_infop info;
png_bytep *row_pointers;
static png_uint_32 png_width;
static png_uint_32 png_height;

int gfx_png_export(char *pngfilename)
{
    printf("Exporting to file '%s'...\n", pngfilename);

    fp = fopen(pngfilename, "wb");
    if(!fp) {
			printf("FP FAIL!\n");
			abort();
			}

    png_init_io(png, fp);

		printf("png_init_io done\n");

    // Output is 8bit depth, RGBA format.

		printf("png_set_ihdr() done\n");

    png_write_info(png, info);

		printf("png_write_info() done\n");

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    //png_set_filler(png, 0, PNG_FILLER_AFTER);


    png_write_image(png, row_pointers);

		printf("png_write_image()\n");

    png_write_end(png, NULL);

		printf("png_write_end()\n");

    for(int y = 0; y < png_height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    fclose(fp);
    return 0;
}

int gfx_png_drawglyph(BitmapFont *font, uint8_t px, uint8_t py, uint8_t glyph, uint8_t fg, uint8_t bg, uint8_t attr)
{
    RGBColour *fgc;
    RGBColour *bgc;
    uint8_t rx = 0;
    uint8_t h = 0;
    SDL_Rect r;
    //printf("gfx_png_drawglyph(%u, %u, %u, %u, '%c')\n", px, py, font->header.px, font->header.py, glyph);
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

						png_bytep row = row_pointers[i];
	 				 png_bytep px = &(row[x * 4]);
            px[0] = 0;
            px[1] = 0;
            px[2] = 0;
            px[3] = 255;


            if (rx & jj) {




//                SDL_SetRenderDrawColor( renderer, fgc->r, fgc->g, fgc->b, 255 );
//                SDL_RenderFillRect( renderer, &r );

                //SDL_RenderDrawPoint(renderer, (px*16) + (h*2), (py*16) + (ii*2));
                //printf("X");
            } else {
//                SDL_SetRenderDrawColor( renderer, bgc->r, bgc->g, bgc->b, 255 );
//                SDL_RenderFillRect( renderer, &r );

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

int gfx_png_main(uint16_t xsize, uint16_t ysize, char *WindowTitle)
{
    uint32_t pngdatasize = 0;

    /* 4 bytes per pixel */

		pngdatasize = sizeof(png_bytep) * ysize;

    printf("[PNG] allocating initial %lu bytes for PNG row pointers\n", (unsigned long) pngdatasize);
    assert(pngdatasize);
    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * ysize);
    assert(row_pointers);
    png_width = xsize;
    png_height = ysize;

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    assert(png);
    info = png_create_info_struct(png);
    assert (info);

  png_set_IHDR(
        png,
        info,
        png_width, png_height,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );


		if (setjmp(png_jmpbuf(png))) abort();	

		printf("allocating data for %u rows\n", png_height);
		printf("png = 0x%08x\n", png);
		printf("info = 0x%08x\n", info);

    for(int y = 0; y < png_height; y++) {
//				printf("png_get_rowbytes(png,info) = %lu\n", png_get_rowbytes(png,info));
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
				
    }

		printf("done\n");
    return 0;
}

int gfx_png_canvas_render(ANSICanvas *canvas, BitmapFont *myfont)
{
    ANSIRaster *r = NULL;
    uint16_t width = 0, height = 0;
    assert(canvas);
    width = canvas_get_width(canvas);
    height = canvas_get_height(canvas);
    printf("gfx_png_canvas_render(%ux%u)\n", png_width, png_height);
		printf("png_width = %u, png_height = %u\n", png_width, png_height);

    for(int y = 0; y < png_height; y++) {
        png_bytep row = row_pointers[y];
        for(int x = 0; x < png_width; x++) {
            png_bytep px = &(row[x * 4]);
            px[0] = 0;
            px[1] = 0;
            px[2] = 0;
            px[3] = 255;
        }
    }


    for (uint16_t ii = 0; ii < height; ii++) {
        r = canvas_get_raster(canvas, ii);
        if (r) {
            for (uint16_t jj = 0; jj < r->bytes; jj++) {
                gfx_png_drawglyph(myfont, jj, ii, r->chardata[jj], r->fgcolors[jj], r->bgcolors[jj], r->attribs[jj]);
            }
        } else {
            printf("canvas data missing for raster %u\n", ii);
        }
    }
    return 0;
}

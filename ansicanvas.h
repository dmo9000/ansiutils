#ifndef __ANSICANVAS_H__

#define __ANSICANVAS_H__

#include "ansiraster.h"

struct ansi_canvas {
    uint16_t lines;
    struct ansi_raster *first_raster;
    int debug_level;
		bool clear_flag;
		bool compress_output;
};

typedef struct ansi_canvas   ANSICanvas;

ANSICanvas *new_canvas();
ANSIRaster *canvas_get_raster(ANSICanvas *canvas, int line);
ANSIRaster *canvas_add_raster(ANSICanvas *canvas);
bool canvas_output(ANSICanvas *canvas, bool use_unicode, char *filename);
uint16_t canvas_get_width(ANSICanvas *canvas);
uint16_t canvas_get_height(ANSICanvas *canvas);


struct display_colour {
    double r;
    double g;
    double b;
};

struct rgb_color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

typedef struct display_colour DisplayColour;
typedef struct display_colour RGBColour;
RGBColour* canvas_displaycolour(uint8_t colour);


#endif /* __ANSICANVAS_H__ */

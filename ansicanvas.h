#ifndef __ANSICANVAS_H__

#define __ANSICANVAS_H__

#include <stdint.h>
#include <stdbool.h>
#include "ansiraster.h"

#define CANVAS_DEBUG_NONE				0
#define CANVAS_DEBUG_CURSOR			1
#define CANVAS_DEBUG_OUTPUT			2

struct ansi_canvas {
    uint16_t lines;
    struct ansi_raster *first_raster;
    int debug_level;
    bool clear_flag;
    bool compress_output;
    bool scroll_on_output;
    uint16_t scroll_limit;
    bool allow_hard_clear;
    bool repaint_entire_canvas;
    bool is_dirty;
    bool cursor_enabled;
    uint16_t default_raster_length;
    /* reserved for future API update; put a hard limit on the canvas size  */
    uint64_t debug_flags;
    uint16_t columns;
    uint16_t rows;
};

typedef struct ansi_canvas   ANSICanvas;

ANSICanvas *new_canvas();
ANSIRaster *canvas_get_raster(ANSICanvas *canvas, int line);
ANSIRaster *canvas_add_raster(ANSICanvas *canvas);
bool canvas_output(ANSICanvas *canvas, bool use_unicode, char *filename);
uint16_t canvas_get_width(ANSICanvas *canvas);
uint16_t canvas_get_height(ANSICanvas *canvas);
int canvas_setdebugflags(uint64_t flags);
bool canvas_is_dirty(ANSICanvas *c);
int canvas_setdimensions(ANSICanvas *canvas, uint16_t columns, uint16_t rows);
int canvas_backfill(ANSICanvas *canvas);

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

int canvas_reindex(ANSICanvas *c);

#define CONSOLE_WIDTH     80
#define CONSOLE_HEIGHT    24

#endif /* __ANSICANVAS_H__ */

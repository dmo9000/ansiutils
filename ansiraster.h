#ifndef __ANSIRASTER_H__

#define __ANSIRASTER_H__

#include "ansicolor.h"

struct ansi_raster {
    uint16_t bytes;
    unsigned char *chardata;
    ansicolor_t   *fgcolors;
    ansicolor_t   *bgcolors;
    struct ansi_raster *next_raster;     /* not required, but handy */
    uint16_t index;
};

typedef struct ansi_raster   ANSIRaster;

bool raster_append_bytes(ANSIRaster *r, unsigned char *data, uint8_t bytes, ansicolor_t fg, ansicolor_t bg, bool debug);
bool raster_append_byte(ANSIRaster *r, unsigned char data, ansicolor_t fg, ansicolor_t bg, bool debug);

ANSIRaster *create_new_raster();
bool raster_output(ANSIRaster *r, bool debug, bool use_unicode);

#endif /* __ANSIRASTER_H__ */

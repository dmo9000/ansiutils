#include "tdf.h"
#include "tdfraster.h"

static int ansi_color_map[8] = {
    0, 4, 2, 6, 1, 5, 3, 7
};


#define OPTIMIZE_OUTPUT

TDFRaster *create_new_raster()
{
    TDFRaster *new_raster = NULL;
    new_raster = malloc(sizeof(TDFRaster));
    assert(new_raster);
    memset(new_raster, 0, sizeof(TDFRaster));
    return new_raster;

}


bool raster_append_bytes(TDFRaster *r, unsigned char *data, uint8_t bytes, ansicolor_t fg, ansicolor_t bg, bool debug)
{

    int ii = 0;

    /* uncomment this assert to filter some dogdy fonts, need to know why though */

    for (ii = 0; ii < bytes; ii++) {
        if (!raster_append_byte(r, data[ii], fg, bg, debug)) {
            return false;
        };
    }
    return true;

}

bool raster_append_byte(TDFRaster *r, unsigned char data, ansicolor_t fg, ansicolor_t bg, bool debug)
{
    TDFRaster *tdr = r;
    unsigned char *chardata_realloc = NULL;
    unsigned char *fgcolors_realloc = NULL;
    unsigned char *bgcolors_realloc = NULL;

    assert(r);
    /* do not push null byte, ever, since it's a string terminator  */

    if (data < 0x20) {
        /* use 'X' instead for now */
        data = 'X';
    }

    assert(data);

    if (!tdr->bytes) {
        tdr->bytes = 1;
        /* TODO - since we're planning not to just printf() anymore,  probably no need to add the extra byte for much longer */
        tdr->chardata = malloc(tdr->bytes+1);
        tdr->fgcolors = malloc(tdr->bytes+1);
        tdr->bgcolors = malloc(tdr->bytes+1);

        /* write the byte and its foreground and background colors */

        tdr->chardata[0] = data;
        tdr->fgcolors[0] = fg;
        tdr->bgcolors[0] = bg;
        tdr->chardata[1] = '\0'; /* NULL terminate, so that the rasters can be printed with C library functions */
        tdr->fgcolors[1] = '\0';
        tdr->bgcolors[1] = '\0';
        return true;
    } else {
        assert(tdr->chardata);
        assert(tdr->fgcolors);
        assert(tdr->bgcolors);
        assert(tdr->bytes);
        tdr->bytes ++;
        if (debug) {
            //    printf("realloc(), tdr->bytes = %d, address = 0x%08x\n", tdr->bytes+1, tdr->chardata);
        }
        chardata_realloc = realloc(tdr->chardata, tdr->bytes+1);
        fgcolors_realloc = realloc(tdr->fgcolors, tdr->bytes+1);
        bgcolors_realloc = realloc(tdr->bgcolors, tdr->bytes+1);

        assert(chardata_realloc);
        assert(fgcolors_realloc);
        assert(bgcolors_realloc);

        tdr->chardata = chardata_realloc;
        tdr->fgcolors = fgcolors_realloc;
        tdr->bgcolors = bgcolors_realloc;
        assert(tdr->bytes-1);
        tdr->chardata[tdr->bytes-1] = data;
        tdr->fgcolors[tdr->bytes-1] = fg;
        tdr->bgcolors[tdr->bytes-1] = bg;
        tdr->chardata[tdr->bytes] = '\0';
        tdr->fgcolors[tdr->bytes] = '\0';
        tdr->bgcolors[tdr->bytes] = '\0';
    }

    return true;
}

bool raster_output(TDFRaster *r, bool debug_mode)
{

    int jj = 0;
    ansicolor_t fg = 0, bg = 0;
    ansicolor_t last_fg, last_bg;
    bool bold = false, last_bold = false;


    for (jj = 0; jj < r->bytes; jj++) {

        last_fg = fg;
        last_bg = bg;
        last_bold = bold;


        fg = r->fgcolors[jj];
        bg = r->bgcolors[jj];

        if (fg >= 0x08) {
            fg -= 0x08;
            /* ANSI control code - hi intensity */
            bold = true;

        } else {
            /* ANSI control code - normal intensity */
            bold = false;
        }

        fg = ansi_color_map[fg];
        bg = ansi_color_map[bg];

        if (debug_mode) {
            printf("[%u/%03u:%c:%X/%X]", jj, r->chardata[jj],
                   r->chardata[jj], r->fgcolors[jj], r->bgcolors[jj]);
        } else {
#ifdef OPTIMIZE_OUTPUT
            if (fg != last_fg || bg != last_bg) {
#endif /* OPTIMIZE_OUTPUT */
                printf((char *) "\x1b\x5b""%u;%um", 40 + bg, 30 + fg);
#ifdef OPTIMIZE_OUTPUT
            }
#endif /* OPTIMIZE_OUTPUT */

            if (bold) {
#ifdef OPTIMIZE_OUTPUT
                if (bold != last_bold) {
#endif /* OPTIMIZE_OUTPUT */
                    printf("\x1b\x5b""1m");
#ifdef OPTIMIZE_OUTPUT
                }
#endif /* OPTIMIZE_OUTPUT */
            } else {
#ifdef OPTIMIZE_OUTPUT
                if (bold != last_bold) {
#endif /* OPTIMIZE_OUTPUT */
                    printf("\x1b\x5b""21m");
#ifdef OPTIMIZE_OUTPUT
                }
#endif /* OPTIMIZE_OUTPUT */
            }
            putchar(r->chardata[jj]);
        }
        //printf("[%u/%u/%u/%u]", jj, r->chardata[jj], r->fgcolors[jj], r->bgcolors[jj]);
    }

    printf("\x1b\x5b""0m");
    return true;

}

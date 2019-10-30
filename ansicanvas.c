#include <math.h>
#include "tdf.h"
#include "ansiraster.h"
#include "tdffont.h"
#include "ansicanvas.h"
#include "displaycolours.h"

RGBColour rgbcolours[16];

bool canvas_is_dirty(ANSICanvas *c)
{
    return c->is_dirty;
}

ANSICanvas *new_canvas()
{

    ANSICanvas *canvas = NULL;
    canvas = malloc(sizeof(ANSICanvas));
    assert(canvas);
    memset(canvas, 0, sizeof(ANSICanvas));
    canvas->clear_flag = false;
    canvas->compress_output = false;
    return canvas;

}

int canvas_reindex(ANSICanvas *canvas)
{
    ANSIRaster *r = NULL;
    uint16_t new_index = 0;
    assert(canvas);
    r = canvas->first_raster;
    assert(r);
    while (r->next_raster) {
        r->index = new_index;
        new_index++;
        r = r->next_raster;
    }
    r->index = new_index;
    canvas->lines = new_index+1;
    return 0;
}

ANSIRaster* canvas_get_raster(ANSICanvas *canvas, int line)
{
    ANSIRaster *r = NULL;
    int raster_count = 0;

    //printf("canvas_get_raster(%u)\n", line);
    assert(canvas);
    r = canvas->first_raster;
    if (!r) {
        /* no rasters */
        //printf("no rasters\n");
        return NULL;
    }

    while (r && raster_count < canvas->lines) {
        //    printf("checking raster with index; %d\n", r->index);
        if (raster_count == line) {
            if (line != r->index) {
                printf("line/index mismatch: %u -> %u\n", line, r->index);
            }
            assert(line == r->index);
            return r;
        }
        r = r->next_raster;
        raster_count++;
    }

    return NULL;
}

ANSIRaster *canvas_add_raster(ANSICanvas *canvas)
{
    ANSIRaster *r = NULL;
    int raster_count = 0;
//    printf("  canvas_add_raster()\n");
    assert(canvas);
    r = canvas->first_raster;
    if (!r) {
        /* no rasters */
        canvas->first_raster = create_new_raster();
        assert((bool) canvas->first_raster);
        canvas->first_raster->bytes = 0;
        canvas->first_raster->chardata = NULL;
        canvas->first_raster->index = raster_count;
        canvas->lines ++;
        canvas->first_raster->next_raster = NULL;
        return canvas->first_raster;
    }

    /* else, subsequent raster */

    while (r->next_raster && raster_count < canvas->lines) {
        r = r->next_raster;
        raster_count++;
    }

    assert(r);
    assert(!r->next_raster);

    r->next_raster = create_new_raster();
    assert(r->next_raster);
    r->next_raster->bytes = 0;
    r->next_raster->chardata = NULL;
    r->next_raster->index = raster_count + 1;
    r->next_raster->next_raster = NULL;
    canvas->lines++;
    return (r->next_raster);
}

bool canvas_output(ANSICanvas *my_canvas, bool use_unicode, char *filename)
{
    ANSIRaster *r = NULL;
    assert(my_canvas);
    FILE *fh = NULL;

    if (filename) {
        fh = fopen(filename, "wb");
    } else {
        fh = stdout;
    }

    if (my_canvas->clear_flag) {
        /* if the clear flag was set, put the cursor to 0, 0 */
        fprintf(fh, "\x1b\x5b""H");
    }

    if (my_canvas->debug_level < 3) {
        for (int ii = 0; ii < my_canvas->lines ; ii++) {
            r = canvas_get_raster(my_canvas, ii);
            assert(r);

            /* if we hit an empty raster, (ie. a raster with 0 bytes),
               assume we are done */

            if (!r->bytes && !r->chardata) {
                /* blank/missing raster */
                fprintf(stderr, "- line %u is missing/truncated\n", ii);
            } else {
                raster_output(r, false, use_unicode, my_canvas->compress_output, fh);
                fputc('\n', fh);
            }
            //putchar('\n');
        }
    } else {

        if (my_canvas->debug_level) {
            for (int ii = 0; ii < my_canvas->lines ; ii++) {
                r = canvas_get_raster(my_canvas, ii);
                assert(r);
                assert(r->chardata);
                assert(r->bytes);
                raster_output(r, true, use_unicode, false, fh);
                fprintf(fh, "\r\n");
            }
        }
    }

    if (filename) {
        fclose(fh);
    }

    return (true);
}


uint16_t canvas_get_width(ANSICanvas *canvas)
{
    ANSIRaster *r = NULL;
    uint16_t width = 0;
    assert(canvas);

    if (canvas->columns) {
        return canvas->columns;
    }

    /* TODO: this is transitional - check if canvas->columns is defined, if not, calculate the maximum width as per the
    	old method see below */

    for (int ii = 0; ii < canvas->lines; ii++) {
        r = canvas_get_raster(canvas, ii);
        if (!r) {
            printf("*** there were less rasters (%u) than expected (%u); did you forget to reindex?\n", ii, canvas->lines);
            assert(r);
        }
        if (r->bytes > width) {
            width = r->bytes;
        }
    }

    return width;

}

uint16_t canvas_get_height(ANSICanvas *canvas)
{
    ANSIRaster *r = NULL;
    uint16_t height = 0;
    assert(canvas);

    if (canvas->rows) {
        return canvas->rows;
    }

    /* TODO: this is transitional - check if canvas->rows is defined, if not, calculate the maximum height as per the
      old method see below */

    for (int ii = 0; ii < canvas->lines; ii++) {
        r = canvas_get_raster(canvas, ii);
        //if (r && ( r->bytes || ii == (canvas->lines -1))) {
        if (r && ( r->bytes )) {
            //if (r) {
            height++;
        }
    }

    //return height-1;
    return height;

}

RGBColour* canvas_displaycolour(uint8_t colour)
{

    double multiplier = 0.45454;

    rgbcolours[colour].r = (uint8_t) (pow(tcolours[colour].r, multiplier) * 255.0);
    rgbcolours[colour].g = (uint8_t) (pow(tcolours[colour].g, multiplier) * 255.0);
    rgbcolours[colour].b = (uint8_t) (pow(tcolours[colour].b, multiplier) * 255.0);

    return (RGBColour*) &rgbcolours[colour];
}

int canvas_backfill(ANSICanvas *canvas)
{
    ANSIRaster *r = NULL;

    fprintf(stderr, "+++ WARNING: canvas_backfill() is deprecated, use canvas_setdimensions() instead\n");

    for (int ii = 0; ii < canvas->lines - 1; ii++) {
        r = canvas_get_raster(canvas, ii);
        assert(r);
        if (!r->bytes) {
            raster_extend_length_to(r, ( canvas_get_width(canvas) ? canvas_get_width(canvas) : CONSOLE_WIDTH));
        }
    }

    return 1;
}

int canvas_setdimensions(ANSICanvas *canvas, uint16_t columns, uint16_t rows)
{

    fprintf(stderr, "canvas_setdimensions(..., %u, %u)\n", columns, rows);
    /* TODO: make this work like canvas_backfill(), only allowing resizing and honoring canvas->columns/rows
    	instead */

    /* for now ... just set the values */

    canvas->columns = columns;
    canvas->rows = rows;

    return 0;
}

#include "tdf.h"
#include "ansiraster.h"
#include "tdffont.h"
#include "ansicanvas.h"

ANSICanvas *new_canvas()
{

    ANSICanvas *canvas = NULL;
    canvas = malloc(sizeof(ANSICanvas));
    assert(canvas);
    memset(canvas, 0, sizeof(ANSICanvas));
    return canvas;

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

bool canvas_output(ANSICanvas *my_canvas, bool use_unicode)
{
    ANSIRaster *r = NULL;
    assert(my_canvas);

    for (int ii = 0; ii < my_canvas->lines ; ii++) {
        r = canvas_get_raster(my_canvas, ii);
        assert(r);

        /* if we hit an empty raster, (ie. a raster with 0 bytes), 
           assume we are done */

        if (!r->bytes && !r->chardata) {
            return true;
            }

        assert(r->bytes);
        assert(r->chardata);
        raster_output(r, false, use_unicode);
        putchar('\n');
    }

    if (my_canvas->debug_level) {
        for (int ii = 0; ii < my_canvas->lines ; ii++) {
            r = canvas_get_raster(my_canvas, ii);
            assert(r);
            assert(r->chardata);
            assert(r->bytes);
            raster_output(r, true, use_unicode);
            printf("\r\n");
        }
    }

    return (true);
}


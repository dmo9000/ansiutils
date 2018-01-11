#include "tdf.h"

TDFCanvas *new_canvas()
{

    int ii = 0;
    TDFCanvas *canvas = NULL;
    canvas = malloc(sizeof(TDFCanvas));
    assert(canvas);
    memset(canvas, 0, sizeof(TDFCanvas));
    return canvas;

}

TDFRaster* canvas_get_raster(TDFCanvas *canvas, int line)
{
    TDFRaster *r = NULL;
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

TDFRaster *canvas_add_raster(TDFCanvas *canvas)
{
    TDFRaster *last_raster = NULL;
    TDFRaster *r = NULL;
    int raster_count = 0;
//    printf("  canvas_add_raster()\n");
    assert(canvas);
    r = canvas->first_raster;
    if (!r) {
        /* no rasters */
        canvas->first_raster = create_new_raster();
        assert(canvas->first_raster);
        canvas->first_raster->bytes = 0;
        canvas->first_raster->chardata = NULL;
        canvas->first_raster->index = raster_count;
        canvas->lines ++;
        canvas->first_raster->next_raster = NULL;
        return canvas->first_raster;
    }

    /* else, subsequent raster */

    while (r->next_raster && raster_count < canvas->lines) {
        last_raster = r;
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

bool canvas_output(TDFCanvas *my_canvas)
{
    TDFRaster *r = NULL;
    int ii = 0;
    assert(my_canvas);

    for (ii = 0; ii < my_canvas->lines ; ii++) {
        r = canvas_get_raster(my_canvas, ii);
        assert(r);
        assert(r->chardata);
        printf("%s\n", r->chardata);
    }

    return (true);
}

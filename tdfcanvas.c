#include "tdf.h"

static int ansi_color_map[8] = {
    0, 4, 2, 6, 1, 5, 3, 7 
};


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
    ansicolor_t fg = 0, bg = 0;
    bool bold = false;
    int ii = 0, jj = 0;
    assert(my_canvas);

    for (ii = 0; ii < my_canvas->lines ; ii++) {
        r = canvas_get_raster(my_canvas, ii);
        assert(r);
        assert(r->chardata);
        assert(r->bytes);
        raster_output(r);
        putchar('\n');
    }

    return (true);
}

bool push_glyph(TDFCanvas *my_canvas, TDFFont *tdf, uint8_t c)
{
    unsigned char dummy_spacing[30];
    TDFCharacter *tdc = NULL;
    TDFRaster *src_raster = NULL;
    TDFRaster *dst_raster = NULL;
    bool is_space = false;
    int ii = 0;
    int jj = 0;

    assert(my_canvas);
    assert(tdf);

    /* FIXME: special handling for space! it would be nice to handle this in a more graceful way */


    if (c == 32) {
        is_space = true;
        memset(&dummy_spacing, 0, 30);
        assert(1 <= tdf->spacing <= 30);
        memset(&dummy_spacing, ' ', tdf->average_width);

        /* fake it, baby */

//        assert(tdf->average_height);
        assert(tdf->maximum_height);

        for (ii = 0; ii < tdf->maximum_height; ii++) {
            dst_raster = canvas_get_raster(my_canvas, ii);

            while (!dst_raster) {
                dst_raster = canvas_add_raster(my_canvas);
            }
            assert(dst_raster);
            assert(ii == dst_raster->index);
            assert(ii <= my_canvas->lines);
            assert(tdf);
            assert(tdf->average_width);
            /* TODO: don't use constants for colors here, use #defines */
            assert(raster_append_bytes(dst_raster, (char*) &dummy_spacing, tdf->average_width, 7, 0, false));
        }
        return true;
    }

    /* otherwise it's not a space */

    memset(&dummy_spacing, 0, 30);
    assert(1 <= tdf->spacing <= 30);
    memset(&dummy_spacing, ' ', tdf->spacing);


    //printf("c = '%c'\n", c);

    c -= 33;
    assert(c >= 0 && c <= 93);
    tdc = &tdf->characters[c];

    assert(tdc);

    /* make sure character will fit on canvas vertically */

    if (tdc->undefined) {
        /* if the glyph is undefined, just skip it */
        return true;
    }


    assert(!tdc->undefined);
    assert(tdc->prerendered);

    if (tdf->maximum_height > MAX_LINES) {
        printf("%s: tdf->maximum_height = %u is larger than MAX_LINES = %u\n", tdf->name, tdf->maximum_height, MAX_LINES);
        exit(1);
    }

    assert(tdf->maximum_height <= MAX_LINES);
    //printf("tdf->maximum_height = %u\n", tdf->maximum_height);
//    for (ii = 0; ii < tdc->height; ii++) {
    for (ii = 0; ii < tdf->maximum_height; ii++) {
        //printf("ii = %u\n", ii);
        assert(tdc);
        assert(tdc->char_rasters[ii]);
        src_raster = tdc->char_rasters[ii];
        //dst_raster = my_canvas->rasters[ii];
        dst_raster = canvas_get_raster(my_canvas, ii);
        while (!dst_raster) {
            dst_raster = canvas_add_raster(my_canvas);
        }
        assert(dst_raster);
        //printf("writing line %u, canvas->lines = %u, dst_raster->index = %u\n", ii, my_canvas->lines, dst_raster->index);
        assert(ii == dst_raster->index);
        assert(ii <= my_canvas->lines);
        assert(src_raster);


        if ((!src_raster->chardata || !src_raster->bytes)) {
            /* the glyph is prerendered, and not a space character,
            	 but this particular raster doesn't have any data.
            	 we fill it with dummy spacing data instead */
            //printf("push_glyph: empty raster %u/%u !!\n", ii, tdc->height);
            //exit(1);
            memset(&dummy_spacing, 0, 30);
            assert(1 <= tdc->width <= 30);
            memset(&dummy_spacing, ' ', tdc->width);
//            assert(raster_append_bytes(dst_raster, (char*) &dummy_spacing, tdf->average_width, 7, 0, false));
            assert(raster_append_bytes(dst_raster, (char*) &dummy_spacing, tdc->width, 7, 0, false));

            /* don't forget the font-level spacing as well */
            memset(&dummy_spacing, 0, 30);
            assert(1 <= tdf->spacing <= 30);
            memset(&dummy_spacing, ' ', tdf->spacing);
            assert(raster_append_bytes(dst_raster, (char*) &dummy_spacing, tdf->spacing, 7, 0, false));
            return true;
        }

        assert(src_raster->chardata);
        assert(src_raster->bytes);
        //printf("dst_raster->bytes = %u\n", dst_raster->bytes);
        //printf("src_raster->bytes = %u\n", src_raster->bytes);

        /* FIXME: its seems we need a copy_raster() function that preserves fg/bg colors, as
                well as a raster_append_space[s]() */

        //assert(raster_append_bytes(dst_raster, src_raster->chardata, src_raster->bytes, 7, 0, false));
        for (jj = 0; jj < src_raster->bytes ; jj++) {
                assert(raster_append_byte(dst_raster, src_raster->chardata[jj], src_raster->fgcolors[jj], src_raster->bgcolors[jj], false));
                } 
        assert(raster_append_bytes(dst_raster, (char*) &dummy_spacing, tdf->spacing, 7, 0, false));
    }
    return true;

}

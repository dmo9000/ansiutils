#include "tdf.h"


TDFRaster *create_new_raster()
{
    TDFRaster *new_raster = NULL;
    new_raster = malloc(sizeof(TDFRaster));
    assert(new_raster);
    memset(new_raster, 0, sizeof(TDFRaster));
    return new_raster;

}


bool raster_append_bytes(TDFRaster *r, unsigned char *data, int bytes, bool debug)
{

    int ii = 0;
    for (ii = 0; ii < bytes; ii++) {
        if (!raster_append_byte(r, data[ii], debug)) {
            return false;
        };
    }
    return true;

}

bool raster_append_byte(TDFRaster *r, unsigned char data, bool debug)
{
    TDFRaster *tdr = r;
    unsigned char *raster_realloc = NULL;

    assert(r);

    if (!tdr->bytes) {
        tdr->bytes = 1;
        tdr->chardata = malloc(tdr->bytes+1);
        tdr->chardata[0] = data;
        tdr->chardata[1] = '\0';                            /* NULL terminate, so that the rasters can be printed with C library functions */
        return true;
    } else {
        assert(tdr->chardata);
        assert(tdr->bytes);
        tdr->bytes ++;
        if (debug) {
            //    printf("realloc(), tdr->bytes = %d, address = 0x%08x\n", tdr->bytes+1, tdr->chardata);
        }
        raster_realloc = realloc(tdr->chardata, tdr->bytes+1);
        assert(raster_realloc);
        tdr->chardata = raster_realloc;
        tdr->chardata[tdr->bytes-1] = data;
        tdr->chardata[tdr->bytes] = '\0';
    }

    return true;
}

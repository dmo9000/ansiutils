#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tdf.h"


TDFRaster *create_new_raster()
{
    TDFRaster *new_raster = NULL;
    new_raster = malloc(sizeof(TDFRaster));
    assert(new_raster);
    memset(new_raster, 0, sizeof(TDFRaster));
    return new_raster;

}
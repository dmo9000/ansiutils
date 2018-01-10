#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tdf.h"


TDFFont *create_new_font()
{
    TDFFont *new_font = NULL;
    new_font = malloc(sizeof(TDFFont));
    assert(new_font);
    memset(new_font, 0, sizeof(TDFFont));
    return new_font;

}
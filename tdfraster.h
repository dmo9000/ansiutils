struct tdf_raster {
    uint16_t bytes;
    unsigned char *chardata;
    ansicolor_t   *fgcolors;
    ansicolor_t   *bgcolors;
    struct tdf_raster *next_raster;     /* not required, but handy */
    uint16_t index;
};

typedef struct tdf_raster   TDFRaster;

bool raster_append_bytes(TDFRaster *r, unsigned char *data, uint8_t bytes, ansicolor_t fg, ansicolor_t bg, bool debug);
bool raster_append_byte(TDFRaster *r, unsigned char data, ansicolor_t fg, ansicolor_t bg, bool debug);

TDFRaster *create_new_raster();
bool raster_output(TDFRaster *r, bool debug, bool use_unicode);


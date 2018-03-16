struct tdf_canvas {
    uint64_t lines;
    struct tdf_raster *first_raster;
    int debug_level;
};

typedef struct tdf_canvas   TDFCanvas;

TDFCanvas *new_canvas();
bool push_glyph(TDFCanvas *my_canvas, TDFFont *tdf, uint8_t c);
TDFRaster *canvas_get_raster(TDFCanvas *canvas, int line);
TDFRaster *canvas_add_raster(TDFCanvas *canvas);
bool canvas_output(TDFCanvas *canvas, bool use_unicode);


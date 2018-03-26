struct ansi_canvas {
    uint64_t lines;
    struct ansi_raster *first_raster;
    int debug_level;
};

typedef struct ansi_canvas   ANSICanvas;

ANSICanvas *new_canvas();
bool push_glyph(ANSICanvas *my_canvas, TDFFont *tdf, uint8_t c);
ANSIRaster *canvas_get_raster(ANSICanvas *canvas, int line);
ANSIRaster *canvas_add_raster(ANSICanvas *canvas);
bool canvas_output(ANSICanvas *canvas, bool use_unicode);


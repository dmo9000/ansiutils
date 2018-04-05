int gfx_png_main(uint16_t, uint16_t);
int gfx_png_drawglyph(BitmapFont *bmf, uint8_t px, uint8_t py, uint8_t glyph, uint8_t fg, uint8_t bg, uint8_t attr);
int gfx_png_export();
int gfx_png_canvas_render(ANSICanvas *canvas, BitmapFont *myfont);


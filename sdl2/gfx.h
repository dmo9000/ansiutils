int gfx_main(uint16_t, uint16_t, char *WindowTitle);
int gfx_drawglyph(BitmapFont *bmf, uint8_t px, uint8_t py, uint8_t glyph, uint8_t fg, uint8_t bg, uint8_t attr);
int gfx_expose();
int gfx_canvas_render(ANSICanvas *canvas, BitmapFont *myfont);


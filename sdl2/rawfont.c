#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include "rawfont.h"
#include "../tdf.h"
#include "../tdffont.h"
#include "../ansiraster.h"
#include "../ansicanvas.h"

#define CANVAS_WIDTH    80
#define CANVAS_HEIGHT   24

ANSICanvas *my_canvas = NULL;
BitmapFont *bmf_load(char *filename);

extern int ansi_read(char *ansi_file_name);
extern int gfx_main(uint16_t, uint16_t, char *WindowTitle);
extern int gfx_drawglyph(BitmapFont *font, uint8_t px, uint8_t py, uint8_t glyph, uint8_t fg, uint8_t bg, uint8_t attr);
extern int gfx_expose();

int main(int argc, char *argv[])
{
    char *filename = (char *) argv[1];
    BitmapFont *myfont;
    filename = "bmf/8x8.bmf";

    myfont = bmf_load(filename);

    if (!myfont) {
        perror("bmf_load");
        exit(1);
    }

    my_canvas = new_canvas();

    gfx_main((CANVAS_WIDTH*8), (CANVAS_HEIGHT*16), "BMF Font Render Test");

    for (int kk = 0; kk < 256; kk++) {
        gfx_drawglyph(myfont, (kk % CANVAS_WIDTH), (kk / CANVAS_WIDTH), kk, 7, 0, ATTRIB_NONE);
    }
//    ansi_read("ansifiles/fruit.ans");


    gfx_expose();
    while (!getchar()) {
    }
    exit(0);

}


BitmapFont *bmf_load(char *filename)
{

    FILE *rawfont = NULL;
    size_t font_len_expect;
    BitmapFont *myfont = NULL;
    int r = 0;
    int rd = 0;

    rawfont = fopen(filename, "rb");
    if (!rawfont) {
        perror("fopen");
        exit(1);
    }

    if (fseek(rawfont, 0, SEEK_SET)) {
        perror("fseek");
        exit(1);
    }

    myfont = (BitmapFont*) malloc(sizeof(BitmapFont));

    r = fread(&myfont->header, sizeof(BitmapFontHeader), 1, rawfont);

    if (r != 1) {
        perror("fread() header");
        exit(1);
    }

    printf("Magic:   [%c%c%c]\n", myfont->header.magic[0], myfont->header.magic[1], myfont->header.magic[2]);
    printf("Version: [%u]\n", myfont->header.version);
    printf("Columns: [%u]\n", myfont->header.px);
    printf("Rows:    [%u]\n", myfont->header.py);
    printf("Glyphs:  [%u]\n", myfont->header.glyphs);

    assert(myfont->header.magic[0] == 'B');
    assert(myfont->header.magic[1] == 'M');
    assert(myfont->header.magic[2] == 'F');
    assert(myfont->header.version == 0);
    assert(myfont->header.px == 8);
    assert(myfont->header.py == 8);
    assert(myfont->header.glyphs == 256);

    font_len_expect = (myfont->header.py * myfont->header.glyphs);

    assert(fseek(rawfont, 0, SEEK_END) == 0);
    myfont->size = ftell(rawfont) - sizeof(BitmapFontHeader);
    assert ((bool) (myfont->size == font_len_expect));
    myfont->fontdata = malloc(myfont->size);

    if (!myfont->fontdata) {
        perror("malloc");
        exit(1);
    }


    printf("expected and found %lu bytes\n", (myfont->size));

    assert(fseek(rawfont, sizeof(BitmapFontHeader), SEEK_SET) == 0);

    while (rd < font_len_expect) {
        r = fread(myfont->fontdata + rd, myfont->header.py, 1, rawfont);
        if (r != 1) {
            perror("fread");
            printf("r = %u\n", r);
            for (int kk = 0; kk < myfont->header.px*myfont->header.py; kk++) {
                printf("%02x ", myfont->fontdata[rd+kk]);
            }
            exit(1);
        }
        rd += (r * (myfont->header.py));
    }

    printf("read %lu bytes\n", (long unsigned int) rd);
    fclose(rawfont);

    return (myfont);
}

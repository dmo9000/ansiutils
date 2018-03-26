#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include "rawfont.h"

#define CANVAS_WIDTH    80
#define CANVAS_HEIGHT   24

extern int gfx_main(uint16_t, uint16_t);
extern int gfx_drawglyph(BitmapFont *bmf, uint8_t px, uint8_t py, uint8_t glyph);
extern int gfx_expose();

int main(int argc, char *argv[])
{
    size_t font_len_expect;
    uint8_t *fontdata = NULL;
    FILE *rawfont = NULL;
    char *filename = (char *) argv[1];
    uint8_t height = 8;
    uint8_t width = 8;
    uint16_t rd = 0, r = 0;
    BitmapFont myfont;

    if (argc < 2) {
            printf("*** using default font ***\n");
            filename = "bmf/8x8.bmf";
            }

    rawfont = fopen(filename, "rb");
    if (!rawfont) {
        perror("fopen");
        exit(1);
    }

    if (fseek(rawfont, 0, SEEK_SET)) {
        perror("fseek");
        exit(1);
    }

    r = fread(&myfont.header, sizeof(BitmapFontHeader), 1, rawfont);

    if (r != 1) {
        perror("fread() header");
        exit(1);
    }

    printf("Magic:   [%c%c%c]\n", myfont.header.magic[0], myfont.header.magic[1], myfont.header.magic[2]);
    printf("Version: [%u]\n", myfont.header.version);
    printf("Columns: [%u]\n", myfont.header.px);
    printf("Rows:    [%u]\n", myfont.header.py);
    printf("Glyphs:  [%u]\n", myfont.header.glyphs);

    assert(myfont.header.magic[0] == 'B');
    assert(myfont.header.magic[1] == 'M');
    assert(myfont.header.magic[2] == 'F');
    assert(myfont.header.version == 0);
    assert(myfont.header.px == 8);
    assert(myfont.header.py == 8);
    assert(myfont.header.glyphs == 256);

    font_len_expect = (myfont.header.py * myfont.header.glyphs);

    assert(fseek(rawfont, 0, SEEK_END) == 0);
    myfont.size = ftell(rawfont) - sizeof(BitmapFontHeader);
    assert ((bool) (myfont.size == font_len_expect));
    myfont.fontdata = malloc(myfont.size);

    if (!myfont.fontdata) {
        perror("malloc");
        exit(1);
    }
    printf("expected and found %lu bytes\n", (myfont.size));

    assert(fseek(rawfont, sizeof(BitmapFontHeader), SEEK_SET) == 0);

    while (rd < font_len_expect) {
        r = fread(myfont.fontdata + rd, width*height, 1, rawfont);
        if (r != 1) {
            perror("fread");
            for (int kk = 0; kk < width*height; kk++) {
                printf("%02x ", fontdata[rd+kk]);
            }
            exit(1);
        }
        rd += (r * (width*height));
    }

    printf("read %lu bytes\n", (long unsigned int) rd);
    fclose(rawfont);
    gfx_main((CANVAS_WIDTH*8), (CANVAS_HEIGHT*16));
    for (int kk = 0; kk < 256; kk++) {
        gfx_drawglyph(&myfont, (kk % CANVAS_WIDTH), (kk / CANVAS_WIDTH), kk);
        }
    gfx_expose();
    while (!getchar()) {
    }
    exit(0);

}

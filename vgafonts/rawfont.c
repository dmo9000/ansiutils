#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include "rawfont.h"

#define MAX_CHARS 256

extern int gfx_main(uint16_t, uint16_t);
extern int gfx_drawglyph(uint8_t *fontdata, uint8_t height, uint16_t px, uint16_t py, uint16_t glyph);


int main(int argc, char *argv[])
{
    size_t font_len_expect;
    size_t font_len_actual;
    uint8_t *fontdata = NULL;
    FILE *rawfont = NULL;
    char *filename = (char *) argv[1];
    uint8_t height = 8;
    uint8_t width = 8;
    uint16_t rd = 0, r = 0;
    BitmapFont myfont;

    /* hardcoded for 8x8 font as first argument for now */

    font_len_expect = (width * height * MAX_CHARS);
    rawfont = fopen(filename, "rb");
    if (!rawfont) {
        perror("fopen");
        exit(1);
    }

    if (fseek(rawfont, 0, SEEK_SET)) {
        perror("fseek");
        exit(1);
    }

    r = fread(&myfont, sizeof(BitmapFont), 1, rawfont);

    if (r != 1) {
        perror("fread() header");
        exit(1);
    }

    printf("Magic:   [%c%c%c]\n", myfont.magic[0], myfont.magic[1], myfont.magic[2]);
    printf("Version: [%u]\n", myfont.version);
    printf("Columns: [%u]\n", myfont.px);
    printf("Rows:    [%u]\n", myfont.py);
    printf("Glyphs:  [%u]\n", myfont.glyphs);

    assert(myfont.version == 0);
    assert(myfont.px == 8);
    assert(myfont.py == 8);
    assert(myfont.glyphs == 256);

    font_len_actual = ftell(rawfont);
    assert ((bool) (font_len_actual == font_len_expect));

    fontdata = malloc(font_len_expect);

    if (!fontdata) {
        perror("malloc");
        exit(1);
    }
    printf("expected and found %lu bytes\n", (font_len_expect));

    while (rd < font_len_expect) {
        printf("rd = %u, reading %u\n", rd, width*height);
        r = fread(fontdata + rd, 1, width*height, rawfont);
        if (!r) {
            perror("fread");
            for (int kk = 0; kk < width*height; kk++) {
                printf("%02x ", fontdata[rd+kk]);
            }
            exit(1);
        }
        printf("r = %u\n", r);
        rd += (r * (width*height));
    }

    printf("read %lu bytes\n", (long unsigned int) rd);

    fclose(rawfont);

    gfx_main(height * (16*2), width * (16*2));

    gfx_drawglyph(fontdata, height, 0, 0, 'A');

    while (!getchar()) {
    }
    exit(0);

}

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include "rawfont.h"


BitmapFont *bmf_embedded(char *data)
{

    size_t font_len_expect;
    BitmapFont *myfont = NULL;

    myfont = (BitmapFont*) malloc(sizeof(BitmapFont));

    memcpy(&myfont->header, data, sizeof(BitmapFontHeader));

    /*
    printf("Magic:   [%c%c%c]\r\n", myfont->header.magic[0], myfont->header.magic[1], myfont->header.magic[2]);
    printf("Version: [%u]\r\n", myfont->header.version);
    printf("Columns: [%u]\r\n", myfont->header.px);
    printf("Rows:    [%u]\r\n", myfont->header.py);
    printf("Glyphs:  [%u]\r\n", myfont->header.glyphs);
    */

    assert(myfont->header.magic[0] == 'B');
    assert(myfont->header.magic[1] == 'M');
    assert(myfont->header.magic[2] == 'F');
    assert(myfont->header.version == 0);
    assert(myfont->header.px == 8);
    assert(myfont->header.py == 8);
    assert(myfont->header.glyphs == 256);

    font_len_expect = (myfont->header.py * myfont->header.glyphs);

    myfont->fontdata = malloc(font_len_expect);

    if (!myfont->fontdata) {
        perror("malloc");
        exit(1);
    }
    memcpy(myfont->fontdata, data + sizeof(BitmapFontHeader), font_len_expect);
    return myfont;

}

BitmapFont *bmf_load(char *filename)
{

    FILE *rawfont = NULL;
    size_t font_len_expect;
    BitmapFont *myfont = NULL;
    int r = 0;
    int rd = 0;

    printf("bmf_load(%s)\n", filename);

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

    /*
    printf("Magic:   [%c%c%c]\r\n", myfont->header.magic[0], myfont->header.magic[1], myfont->header.magic[2]);
    printf("Version: [%u]\r\n", myfont->header.version);
    printf("Columns: [%u]\r\n", myfont->header.px);
    printf("Rows:    [%u]\r\n", myfont->header.py);
    printf("Glyphs:  [%u]\r\n", myfont->header.glyphs);
    */

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


//    printf("expected and found %lu bytes\r\n", (myfont->size));

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

//   printf("read %lu bytes\r\n", (long unsigned int) rd);
    fclose(rawfont);

    return (myfont);
}

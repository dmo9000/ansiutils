#ifndef __RAWFONT_H__

#define __RAWFONT_H__


struct bitmapfontheader {
    unsigned char magic[3];
    uint8_t version;
    uint8_t px;
    uint8_t py;
    uint16_t glyphs;
};

struct bitmapfont {
    struct bitmapfontheader header;
    size_t size;
    unsigned char *fontdata;
};

typedef struct bitmapfontheader BitmapFontHeader;
typedef struct bitmapfont BitmapFont;

#endif /* __RAWFONT_H__ */


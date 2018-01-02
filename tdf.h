#include <stdint.h>

#define TDF_MAGIC_SIZE          20
#define TDF_FONTMARKER_SIZE     4
#define TDF_MAXCHAR             94
#define TDF_ASCII_LO            33
#define TDF_ASCII_HI            126

#define TYPE_OUTLINE            0
#define TYPE_BLOCK              1
#define TYPE_COLOR              2

#define MAX_NAMELEN             12

struct tdf_char;
struct tdf_font;
struct tdf;

struct tdf_char {
    uint8_t ascii_value;
    uint16_t offset;
    unsigned char *fontdata;
    uint16_t datasize;
    struct tdf_font *parent_font;
    uint8_t width; 
    uint8_t height;
    uint8_t type;
};


struct tdf_font {
    char *name;
    unsigned char *data;
    uint8_t type;
    uint8_t spacing;
    uint32_t offset;
    uint16_t blocksize;
    uint16_t references;
    struct tdf_char characters[TDF_MAXCHAR];
    struct tdf_font *next_font;
    struct tdf *parent_tdf;
};

struct tdf {
    unsigned char tdfmagic[TDF_MAGIC_SIZE];
    uint8_t fontcount;
    struct tdf_font *first_font;
    FILE *fh;
    off_t limit;
    int debug_level;
};


int ansi_color_map[8] = {
                        0, 4, 2, 6, 1, 5, 3, 7
                        };

bool push_font(struct tdf *my_tdf, struct tdf_font *new_font);
const char *get_font_name(struct tdf *my_tdf, int id);
struct tdf_font* getfont_by_id(struct tdf *my_tdf, int id);
bool render_glyph(struct tdf_font *render_font, unsigned c);
bool emit_glyph(struct tdf_font *font, unsigned char *data);
const char *get_font_type(int type);


struct tdf_char {
    uint8_t ascii_value;
    uint16_t offset;
    unsigned char *fontdata;
    uint16_t datasize;
    struct tdf_font *parent_font;
    uint8_t width;
    uint8_t height;
    uint8_t discovered_height;
    uint8_t type;
    bool prerendered;
    bool undefined;
    struct tdf_raster *char_rasters[MAX_LINES];
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
    uint8_t average_width;
    uint8_t average_height;
    uint8_t maximum_height;
    uint8_t defined_characters;
};

typedef struct tdf_char     TDFCharacter;
typedef struct tdf_font     TDFFont;

bool push_font(struct tdf *my_tdf, struct tdf_font *new_font);
const char *get_font_name(struct tdf *my_tdf, int id);
struct tdf_font* getfont_by_id(struct tdf *my_tdf, int id);
bool render_glyph(struct tdf_font *render_font, unsigned c);
bool prerender_glyph(TDFFont *font, unsigned char c);
const char *get_font_type(int type);
bool display_glyph(TDFFont *tdf, uint8_t c, bool use_unicode);
TDFFont *create_new_font();


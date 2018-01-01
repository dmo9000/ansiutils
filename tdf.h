#include <stdint.h>

#define TDF_MAGIC_SIZE          20
#define TDF_FONTMARKER_SIZE     4
#define TDF_MAXCHAR             94

#define MAX_NAMELEN             12

struct tdf_char {
                uint8_t ascii_value;
                uint16_t offset;
                };


struct tdf_font {
            char *name;
            uint8_t type;
            uint8_t spacing; 
            uint16_t blocksize;
            uint16_t references;
            struct tdf_char characters[TDF_MAXCHAR];
            struct tdf_font *next_font;
            };  

struct tdf {
            unsigned char tdfmagic[TDF_MAGIC_SIZE];
            uint8_t fontcount;
            struct tdf_font *first_font;
           };


bool push_font(struct tdf *my_tdf, struct tdf_font *new_font);
const char *get_font_name(struct tdf *my_tdf, int id);
struct tdf_font* getfont_by_id(struct tdf *my_tdf, int id);

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>



#define NO_ANSI_IN_RASTER


#define TDF_MAGIC_SIZE          20
#define TDF_FONTMARKER_SIZE     4
#define TDF_MAXCHAR             94
#define TDF_ASCII_LO            33
#define TDF_ASCII_HI            126

#define TYPE_OUTLINE            0
#define TYPE_BLOCK              1
#define TYPE_COLOR              2

#define MAX_NAMELEN             12
#define MAX_LINES               13  /* this is more like it */

#define MAX_ANSI_SEQUENCE       32

struct ansi_raster;
struct ansi_canvas;
struct tdf_char;
struct tdf_font;
struct tdf;


typedef uint8_t color_t;
typedef color_t ibmcolor_t;
typedef color_t ansicolor_t;

struct tdf {
    unsigned char tdfmagic[TDF_MAGIC_SIZE];
    uint8_t fontcount;
    struct tdf_font *first_font;
    FILE *fh;
    off_t limit;
    int debug_level;
};

typedef struct tdf          TDFHandle;


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <ctype.h>
#include "tdf.h"
#include "osdep.h"


#define create_new_font() malloc(sizeof(struct tdf_font))

int main(int argc, char *argv[])
{
    FILE *tdf_file = NULL;
    char *input_filename = NULL;
    struct tdf_font *new_font = NULL;
    int rd = 0;
    struct tdf my_tdf;
    uint32_t font_sequence_marker;
    uint8_t namelen = 0;
    uint32_t reserved1 = 0;
    uint32_t current_offset = 0;
    int ii = 0;
    bool all_fonts_loaded = false;
    int selected_font = 1;
    bool unicode_output = false;
    char *output_filename = NULL;
    int8_t c = 0;
    char *message = NULL;
    struct tdf_font *render_font = NULL;

    while ((c = getopt (argc, argv, "f:uo:")) != -1) {
        switch (c)
        {
        case 'f':
            selected_font = atoi(optarg);
            break;
        case 'u':
            /* enable UNICODE. Otherwise we default to CP437 */
            unicode_output = true;
            break;
        case 'o':
            /* output filename */
            output_filename = strdup((char *) optarg);
            break;
        case '?':
            if (optopt == 'c') {
                printf ("Option -%c requires an argument.\n", optopt);
                exit(1);
            }
            else if (isprint (optopt)) {
                printf ("Unknown option `-%c'.\n", optopt);
                exit(1);
            }
            else {
                printf ("Unknown option character `\\x%x'.\n",
                        optopt);
                exit(1);
            }
            printf("Shouldn't reach here.\n");
            exit(1);
            break;
        case -1:
            /* END OF ARGUMENTS? */
            break;
        default:
            printf("exiting with c= %d [%c]\n", c, c);
            exit (1);
        }

    }

    input_filename = (char *) argv[optind];

    optind++;

    if (optind < argc) {
        message = strdup(argv[optind]);
    }

    tdf_file = fopen(input_filename, "rb");


    if (!tdf_file) {
        printf("Error opening %s\n", input_filename);
        exit(1);
    }

    rd = fread((char *) &my_tdf.tdfmagic, TDF_MAGIC_SIZE, 1, tdf_file);
    if (rd != 1) {
        printf("couldn't read %u bytes header from %s - file truncated?\n", tdf_file);
        perror("fread");
        exit(1);
    }

    if (memcmp(&my_tdf.tdfmagic, "\x13TheDraw FONTS file\x1a", 20) == 0) {
//        printf("* TDF Header OK!\n\n");
    } else {
        printf("Bad TDF header.\n");
        exit(1);
    }

    my_tdf.fh = tdf_file;

    my_tdf.first_font = NULL;

    /* get size, then rewind to start */

    fseek(my_tdf.fh, 0, SEEK_END);
    my_tdf.limit = ftell(my_tdf.fh);
    fseek(my_tdf.fh, TDF_MAGIC_SIZE, SEEK_SET);

    while (!all_fonts_loaded) {
        /* should be at byte 20 byte by now */
        current_offset = ftell(my_tdf.fh);

        if (current_offset >= my_tdf.limit) {
            all_fonts_loaded = true;
            break;
        }

        //printf("* current offset = 0x%04x/0x%04x, checking for font header\n", current_offset, max_offset);

        if (fread((uint32_t*) &font_sequence_marker, TDF_FONTMARKER_SIZE, 1, my_tdf.fh) != 1) {
            printf("Couldn't read font sequence marker at offset 0x%04x.\n", current_offset);
            exit(1);
        } else {

            if (!IS_BIG_ENDIAN) {
                font_sequence_marker = ntohl(font_sequence_marker);
            }

            //printf("FSM = 0x%04x\n", font_sequence_marker);
            my_tdf.fontcount++;
            new_font = create_new_font();
            new_font->data = NULL;
            new_font->parent_tdf = &my_tdf;
            new_font->next_font = NULL;

            assert(new_font);
            assert(font_sequence_marker == 0x55aa00ff);

            if (fread((uint8_t*) &namelen, 1, 1, my_tdf.fh) != 1) {
                printf("Couldn't read font name length.\n");
                exit(1);
            };

            //printf("font name length = %u\n", namelen);

            assert(namelen);

            new_font->name = malloc(namelen+1);
            memset(new_font->name, 0, namelen+1);

            if (fread(new_font->name, MAX_NAMELEN, 1, my_tdf.fh) != 1) {
                printf("Error reading font name (%u bytes)\n", namelen);
                exit(1);
            }

            printf("  %2u [%-12s]\n", my_tdf.fontcount, new_font->name);
            if (fread(&reserved1, sizeof(uint32_t), 1, my_tdf.fh) != 1) {
                printf("Failure reading reserved1\n");
                exit(1);
            }

            assert(!reserved1);

            //printf("reserved1 ok\n");

            if (fread(&new_font->type, sizeof(uint8_t), 1, my_tdf.fh) != 1) {
                printf("couldn't read font type\n");
            }
            assert(new_font->type >= 0);
            assert(new_font->type <= 2);
            //printf("font type = %u\n", new_font->type);

            if (fread(&new_font->spacing, sizeof(uint8_t), 1, my_tdf.fh) != 1) {
                printf("couldn't read spacing\n");
            }

            assert(new_font->spacing >= 0);
            assert(new_font->spacing <= 40);
            //printf("letter spacing = %u\n", new_font->spacing);

            if (fread(&new_font->blocksize, sizeof(uint16_t), 1, my_tdf.fh) != 1) {
                printf("couldn't read blocksize\n");
            }

            /* bit naive, these will always pass */

            if (IS_BIG_ENDIAN) {
                new_font->blocksize = ntohs(new_font->blocksize);
            }

            assert(new_font->blocksize >= 0);
            assert(new_font->blocksize <= 65535);

            // printf("blocksize = %u\n", new_font->blocksize);
            /* store the location of the data segment */

            for (ii = 0 ; ii < TDF_MAXCHAR; ii++) {
                new_font->characters[ii].ascii_value = 33 + ii;
                if (fread((uint16_t*) &new_font->characters[ii].offset, sizeof(uint16_t), 1, my_tdf.fh) != 1) {
                    printf("failed to get character %u offset\n", ii);
                    exit(1);
                }
                if (new_font->characters[ii].offset == 0xFFFF) {
                    //printf("character %u '%c' is not defined\n", ii, new_font->characters[ii].ascii_value);
                } else {
                    /*
                    printf("character %u '%c' is defined at offset 0x%04x\n", ii,
                            new_font->characters[ii].ascii_value,
                            new_font->characters[ii].offset);
                    */
                    new_font->references++;
                }
                new_font->characters[ii].ascii_value = 33 + ii;
                /* don't read font data now, we'll do it lazily when we render the glyph */
                new_font->characters[ii].fontdata = NULL;
                new_font->characters[ii].parent_font = new_font;
            }
            //printf("+ Loaded %u character references\n", new_font->references);
            /* store the location of the data segment */
            new_font->offset = ftell(my_tdf.fh);
            if (!push_font(&my_tdf, new_font)) {
                printf("! error storing font reference\n");
                exit(1);
            }
        }
        /* seek to next font, repeat */
        //printf("... skipping %u bytes ...\n", new_font->blocksize);
        fseek(my_tdf.fh, new_font->blocksize, SEEK_CUR);
    }

    //printf("\n* %u fonts/variations found\n", my_tdf.fontcount);

    if (!(selected_font > 0 && selected_font <= my_tdf.fontcount)) {
        printf("Selected font number %d is invalid.\n", selected_font);
        exit(1);
    }


    if (message) {
        /* render glyphs */
        printf("Message to display: %s\n", message);
        render_font = getfont_by_id(&my_tdf, selected_font);
        if (render_font) {
            printf("Using font: %s\n", render_font->name);
        } else {
            printf("Couldn't get font for rendering\n");
            exit(1);
        }

        render_glyph(render_font, 'A');
        render_glyph(render_font, 'B');
        render_glyph(render_font, 'C');
        render_glyph(render_font, 'D');
        render_glyph(render_font, 'E');
    }

    fclose(my_tdf.fh);

}

bool render_glyph(struct tdf_font *render_font, unsigned c)
{

    unsigned char *font_data = NULL;
    uint32_t glyph_offset = 0;
    int rc = 0;

    if (!render_font) {
        return false;
    }

    if (!(c>= TDF_ASCII_LO && c<= TDF_ASCII_HI)) {
        /* glyph index is out of range for a TDF font */
        return false;
    }

    /* get the correct glyph index from the ASCII char value */

    c -= 33;

    /* check info */

    printf(" glyph ascii char: %c\n", render_font->characters[c].ascii_value);
    printf(" font data offset: %u\n", render_font->offset);
    printf("glyph data offset: %u\n", render_font->characters[c].offset);

    glyph_offset = (uint32_t) (render_font->offset + ((uint32_t) render_font->characters[c].offset));

    printf("      data offset: %u + %u = %u\n",
           render_font->offset, (uint32_t) render_font->characters[c].offset, glyph_offset);

    if (!render_font->data) {
        /* load the font from the file now */
        rc = fseek(render_font->parent_tdf->fh,render_font->offset, SEEK_SET);
        printf("file position is now: %ld\n", ftell(render_font->parent_tdf->fh));
        assert (rc != -1);
        render_font->data = malloc(render_font->blocksize);
        rc = fread(render_font->data, render_font->blocksize, 1, render_font->parent_tdf->fh);
        assert(rc == 1);
    }

    //rc = fseek(render_font->parent_tdf->fh, glyph_offset, SEEK_SET);

    emit_glyph(render_font, render_font->data + render_font->characters[c].offset);
    return true;

}


bool emit_glyph(struct tdf_font *font, unsigned char *data)
{
    unsigned char *ptr = data;
    uint8_t width = 0;
    uint8_t height = 0;
    uint8_t type = 0;
    off_t offset = 0;
    uint32_t limit = font->parent_tdf->limit;
    uint8_t byteval = 0;
    uint8_t color = 0;
    uint8_t bg = 0;
    uint8_t fg = 0;
    uint8_t x = 0;
    uint8_t y = 0;
    bool suppress = false;


    width = ptr[0];
    height = ptr[1];
    type = font->type;
    ptr +=3;

    printf("[width = %u, height = %u, type = %u (%s)]\n", width, height, type, get_font_type(type));

    assert(1 <= width <= 30);
    assert(1 <= height <= 12);

    while (ptr[0] != '\0' && offset < limit) {

        switch(type) {
        case TYPE_OUTLINE:
            printf("+ Unhandled font_type = %d\n", type);
            return false;
            ptr++;
            offset++;
            break;
        case TYPE_BLOCK:
            byteval = ptr[0];
            x++;
            //printf("<%d>", x);
            if (!suppress) {
                if (byteval >= 32) {
                    putchar(byteval);
                    putchar(' ');
                } else {
                    if (byteval != 0x0D) {
                        printf("%02x", byteval);
                    }
                }
            }
            if (x > width) {
                //printf("<H>\n");
                printf("\n"); 
                x = 0;
                y++;
            }
            if (byteval == 0x0d) {
                //printf("<S>");
            }

            ptr++;
            offset++;
            break;

        case TYPE_COLOR:
            byteval = ptr[0];
            color = ptr[1];
            bg = color;
            bg = bg & 0x0F;
            fg = color;
            fg = (fg & 0xF0) % 0x0F;
//                printf("[0x%02x][0x%02x] [%02x][%02x] %c\n", byteval, color, bg, fg)
            if (byteval >= 33) {
                putchar(byteval);
                putchar(' ');
            } else {
                if (byteval != 0x0D) {
                    printf("%02x", byteval);
                }
                //putchar(' ');
            }
            x++;
            if (x >= width) {
                printf("<H>\n");
                suppress = false;
                x = 0;
                y++;
            }
            if (byteval == 0x0d) {
                printf("<S>\n");
                suppress = true;
            }

            ptr += 2;
            offset +=2;
            break;
        default:
            printf("+ Unhandled font_type = %d\n", type);
            return false;
            break;
        }
    }

    printf("\n");
    printf("(%d bytes)\n", offset);
    return true;

}


const char *get_font_name(struct tdf *my_tdf, int id)
{

    struct tdf_font *fontptr = getfont_by_id(my_tdf, id);
    if (!fontptr) {
        return "MISSING FONT";
    }

    return (const char *) fontptr->name;
}


struct tdf_font* getfont_by_id(struct tdf *my_tdf, int id)
{
    int icount = 1;
    struct tdf_font *fontptr = my_tdf->first_font;

    while (fontptr && icount <= my_tdf->fontcount) {
        if (icount == id) {
            return fontptr;
        }
        fontptr = fontptr->next_font;
        icount++;
    }

    return fontptr;
}

bool push_font(struct tdf *my_tdf, struct tdf_font *new_font)
{
    struct tdf_font *last_fontptr = NULL;
    struct tdf_font *next_fontptr = NULL;
    int count = 1;

    if (my_tdf && new_font) {
        if (!my_tdf->first_font) {
            my_tdf->first_font = new_font;
            return true;
        } else {
            next_fontptr = my_tdf->first_font;
            while (next_fontptr) {
                last_fontptr = next_fontptr;
                next_fontptr = next_fontptr->next_font;
                count++;
            }
            last_fontptr->next_font = new_font;
            return true;
        }
    }
    return false;
}


const char *get_font_type(int type)
{

    switch(type) {
    case 0:
        return (const char *) "TYPE_OUTLINE";
        break;
    case 1:
        return (const char *) "TYPE_BLOCK";
        break;
    case 2:
        return (const char *) "TYPE_COLOR";
        break;
    }

    return (const char *) "BADFONTTYPE";
}

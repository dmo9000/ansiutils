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


#define create_new_font()   malloc(sizeof(TDFFont))
#define create_new_raster() malloc(sizeof(TDFRaster))

int main(int argc, char *argv[])
{
    FILE *tdf_file = NULL;
    char *input_filename = NULL;
    TDFFont *new_font = NULL;
    int rd = 0;
    TDFHandle my_tdf;
    uint32_t font_sequence_marker;
    uint8_t namelen = 0;
    uint32_t reserved1 = 0;
    uint32_t current_offset = 0;
    int ii = 0, jj = 0;
    bool all_fonts_loaded = false;
    int selected_font = 1;
    bool unicode_output = false;
    char *output_filename = NULL;
    int8_t c = 0;
    char *message = NULL;
    TDFFont *render_font = NULL;
    int debug_level = 0;
    bool vertical = false;
    TDFCanvas *my_canvas = NULL;
    uint16_t running_average_width = 0;
    uint16_t running_average_height = 0;

    while ((c = getopt (argc, argv, "f:uo:dv")) != -1) {
        switch (c)
        {
        case 'd':
            if (debug_level < 4) {
                debug_level++;
            }
            break;
        case 'v':
            vertical = true;
            break;
        case 'f':
            selected_font = atoi(optarg);
            break;
        case 'u':
            /* enable UNICODE. Otherwise we default to CP437 */
            unicode_output = true;
            printf("UNICODE output not currently supported.\n");
            exit(1);
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
    my_tdf.debug_level = debug_level;

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
            new_font->average_width = 0;
            new_font->average_height = 0;
            new_font->defined_characters = 0;


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

            if (my_tdf.debug_level) {
                printf("  %2u [%-12s]\n", my_tdf.fontcount, new_font->name);
            }

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

            if (my_tdf.debug_level) {
                printf("spacing: %u\n", new_font->spacing);
            }

            if (fread(&new_font->blocksize, sizeof(uint16_t), 1, my_tdf.fh) != 1) {
                printf("couldn't read blocksize\n");
            }

            /* bit naive, these will always pass */

            if (IS_BIG_ENDIAN) {
                new_font->blocksize = ntohs(new_font->blocksize);
            }

            assert(new_font->blocksize >= 0);
            assert(new_font->blocksize <= 65535);

            /* store the location of the data segment */

            for (ii = 0 ; ii < TDF_MAXCHAR; ii++) {
                new_font->characters[ii].ascii_value = 33 + ii;
                new_font->characters[ii].undefined = false;
                if (fread((uint16_t*) &new_font->characters[ii].offset, sizeof(uint16_t), 1, my_tdf.fh) != 1) {
                    printf("failed to get character %u offset\n", ii);
                    exit(1);
                }
                /* setup empty rasters */
                for (jj = 0; jj < MAX_LINES; jj++) {
                    new_font->characters[ii].rasters[jj] = create_new_raster();
                    new_font->characters[ii].rasters[jj]->bytes = 0;
                    new_font->characters[ii].rasters[jj]->data = NULL;

                }

//                assert(new_font->characters[ii].offset != 0xFFFF);

                if (new_font->characters[ii].offset == 0xFFFF) {
                    //printf("character %u '%c' is not defined\n", ii, new_font->characters[ii].ascii_value);
                    new_font->characters[ii].undefined = true;
                } else {
                    new_font->references++;
                }
                new_font->characters[ii].ascii_value = 33 + ii;
                /* don't read font data now, we'll do it lazily when we render the glyph */
                new_font->characters[ii].fontdata = NULL;
                new_font->characters[ii].parent_font = new_font;
                new_font->characters[ii].rendered = false;
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
        if (my_tdf.debug_level) {
            printf("Message to display: %s\n", message);
        }
        render_font = getfont_by_id(&my_tdf, selected_font);
        if (render_font) {
            if (my_tdf.debug_level) {
                printf("Using font: %s\n", render_font->name);
            }
        } else {
            printf("Couldn't get font for rendering\n");
            exit(1);
        }

        /* prerender all glyphs */

        for (ii = 33; ii <= 126; ii++) {
            if (render_glyph(render_font, ii)) {
                /* if it was a valid glyph, add it's width to the running average */
                running_average_width += render_font->characters[ii-33].width;
                running_average_height += render_font->characters[ii-33].discovered_height;
                /* don't trust the height value provided in the TDF file, use what we counted while pre-rendering */
                render_font->defined_characters++;
            }
        }

        render_font->average_width = (uint8_t) ((uint16_t) running_average_width / render_font->defined_characters);
        render_font->average_height = (uint8_t) ((uint16_t) running_average_height / render_font->defined_characters);
        /*
        printf("defined characters = %u\n", render_font->defined_characters);
        printf("average_width      = %u\n", render_font->average_width);
        printf("average_height      = %u\n", render_font->average_height);
        */

        /* sanity check */

        assert(render_font->defined_characters);
        assert(render_font->average_width);
        assert(render_font->average_height);


        if (vertical) {
            /* FIXME: use the canvas later - this is just to test that rendering is working */
            for (ii = 0; ii < strlen(message) ; ii++)  {
                display_glyph(render_font, message[ii]);
            }
            exit(0);
        }

        my_canvas = new_canvas();
        for (ii = 0; ii < strlen(message); ii++) {
            //printf("+++++ PUSHING GLYPH %u ['%c']\n", ii, message[ii]);
            fflush(NULL);
            assert(push_glyph(my_canvas, render_font, message[ii]));
        }


        canvas_output(my_canvas);

    }

    fclose(my_tdf.fh);

    /* reset colours */

    printf("\x1b\x5b""0m");

    exit(0);

}


bool push_glyph(TDFCanvas *my_canvas, TDFFont *tdf, uint8_t c)
{
    unsigned char spacing[30];
    TDFCharacter *tdc = NULL;
    TDFRaster *src_raster = NULL;
    TDFRaster *dst_raster = NULL;
    bool is_space = false;
    int ii = 0;

    assert(my_canvas);
    assert(tdf);

    /* special handling for space! */


    if (c == 32) {
        is_space = true;
        memset(&spacing, 0, 30);
        assert(1 <= tdf->spacing <= 30);
        memset(&spacing, ' ', tdf->average_width);

        /* fake it, baby */

        assert(tdf->average_height);

        for (ii = 0; ii < tdf->average_height; ii++) {
            dst_raster = canvas_get_raster(my_canvas, ii);

            while (!dst_raster) {
                dst_raster = canvas_add_raster(my_canvas);
            }
            assert(dst_raster);
            assert(ii == dst_raster->index);
            assert(ii <= my_canvas->lines);
            assert(tdf);
            assert(tdf->average_width);

            assert(raster_append_bytes(dst_raster, (char*) &spacing, tdf->average_width, false));
        }
        return true;
    }

    memset(&spacing, 0, 30);
    assert(1 <= tdf->spacing <= 30);
    memset(&spacing, ' ', tdf->spacing);

    c -= 33;
    //assert(c >= 0 && c <= 93);
    tdc = &tdf->characters[c];

    /* make sure character will fit on canvas vertically */

    for (ii = 0; ii < tdc->height; ii++) {
        src_raster = tdc->rasters[ii];
        //dst_raster = my_canvas->rasters[ii];
        dst_raster = canvas_get_raster(my_canvas, ii);
        while (!dst_raster) {
            dst_raster = canvas_add_raster(my_canvas);
        }
        assert(dst_raster);
        //printf("writing line %u, canvas->lines = %u, dst_raster->index = %u\n", ii, my_canvas->lines, dst_raster->index);
        assert(ii == dst_raster->index);
        assert(ii <= my_canvas->lines);
        assert(src_raster);
        assert(src_raster->bytes);
        assert(src_raster->data);
        //printf("dst_raster->bytes = %u\n", dst_raster->bytes);
        //printf("src_raster->bytes = %u\n", src_raster->bytes);
        assert(raster_append_bytes(dst_raster, src_raster->data, src_raster->bytes, false));
        assert(raster_append_bytes(dst_raster, (char*) &spacing, tdf->spacing, false));
    }
    return true;

}

bool display_glyph(TDFFont *tdf, uint8_t c)
{
    TDFCharacter *tdc = NULL;
    TDFRaster *tdr = NULL;
    int ii = 0;

    if (tdf->parent_tdf->debug_level) {
        printf("display_glyph('%c')\n", c);
    }

    if (!tdf) return false;

    c -= 33;
    assert(c >= 0 && c <= 93);
    tdc = &tdf->characters[c];

    assert(tdc);

    if (tdc->undefined) {
        return false;
    }

    assert(!tdc->undefined);
    assert(tdc->rendered);

    /* get correct character */
    for (ii = 0; ii < tdc->height; ii++) {
        assert (ii < MAX_LINES);
        tdr = tdc->rasters[ii];
        assert(tdr);
        assert(tdr->bytes);
        assert(tdr->data);
        printf("%s (%u,%d)\n", tdr->data, tdr->bytes, (tdr->bytes ? strlen(tdr->data): -1));
    }

    return true;
}


bool raster_append_byte(TDFRaster *r, unsigned char data, bool debug)
{
    TDFRaster *tdr = r;
    unsigned char *raster_realloc = NULL;

    if (!tdr->bytes) {
        assert(!tdr->data);
        tdr->bytes = 1;
        tdr->data = malloc(tdr->bytes+1);
        tdr->data[0] = data;
        tdr->data[1] = '\0';                            /* NULL terminate, so that the rasters can be printed with C library functions */
        return true;
    } else {
        assert(tdr->data);
        assert(tdr->bytes);
        tdr->bytes ++;
        if (debug) {
            //    printf("realloc(), tdr->bytes = %d, address = 0x%08x\n", tdr->bytes+1, tdr->data);
        }
        raster_realloc = realloc(tdr->data, tdr->bytes+1);
        tdr->data = raster_realloc;
        tdr->data[tdr->bytes-1] = data;
        tdr->data[tdr->bytes] = '\0';
    }

    return true;
}

bool raster_append_bytes(TDFRaster *r, unsigned char *data, int bytes, bool debug)
{

    int ii = 0;
    for (ii = 0; ii < bytes; ii++) {
        if (!raster_append_byte(r, data[ii], debug)) {
            return false;
        };
    }
    return true;

}

bool render_glyph(TDFFont *render_font, unsigned c)
{
    TDFCharacter *tdc = NULL;
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
    assert(c >= 0 && c <= 93);
    tdc = &render_font->characters[c];
    assert (tdc);

    if (tdc->undefined) {
        return false;
    }

    /* already done */
    if (tdc->rendered) {
        return true;
    }

    /* check info */

    if (render_font->parent_tdf->debug_level) {
        printf(" glyph ascii char: %c\n", render_font->characters[c].ascii_value);
        printf(" font data offset: %u\n", render_font->offset);
        printf("glyph data offset: %u\n", render_font->characters[c].offset);
    }

    glyph_offset = (uint32_t) (render_font->offset + ((uint32_t) render_font->characters[c].offset));


    if (render_font->parent_tdf->debug_level) {
        printf("      data offset: %u + %u = %u\n",
               render_font->offset, (uint32_t) render_font->characters[c].offset, glyph_offset);
    }

    if (!render_font->data) {
        /* load the font from the file now */
        rc = fseek(render_font->parent_tdf->fh,render_font->offset, SEEK_SET);
        if (render_font->parent_tdf->debug_level) {
            printf("file position is now: %ld (0x%04x)\n",
                   ftell(render_font->parent_tdf->fh),
                   ftell(render_font->parent_tdf->fh));
        }
        assert (rc != -1);
        render_font->data = malloc(render_font->blocksize);
        rc = fread(render_font->data, render_font->blocksize, 1, render_font->parent_tdf->fh);
        assert(rc == 1);
    }

    //rc = fseek(render_font->parent_tdf->fh, glyph_offset, SEEK_SET);

    //prerender_glyph(render_font, render_font->data + render_font->characters[c].offset);
    prerender_glyph(render_font, c);

    return true;

}


bool prerender_glyph(TDFFont *font, unsigned char c)
{
    static char ansi_buffer[MAX_ANSI_SEQUENCE];
    unsigned char *ptr = NULL;
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
    TDFRaster *r = NULL;
    TDFCharacter *tdc = &font->characters[c];

    ptr = font->data + tdc->offset;

    width = ptr[0];
    height = ptr[1];
    type = font->type;
    ptr +=2;

    if (font->parent_tdf->debug_level) {
        printf("[width = %u, height = %u, type = %u (%s)]\n", width, height, type, get_font_type(type));
    }

    assert(1 <= width <= 30);
    assert(1 <= height <= 12);

    while (ptr[0] != '\0' && offset < limit) {

        switch(type) {
        case TYPE_OUTLINE:
            //printf("+ Unhandled font_type = %d\n", type);
            assert(type != TYPE_OUTLINE); 
            return false;
            break;
        case TYPE_BLOCK:
            byteval = ptr[0];
            x++;
            assert(y < MAX_LINES);
            r = tdc->rasters[y];
            assert(r);
            if (!suppress) {
                if (byteval >= 32) {
                    //putchar(byteval);
                    raster_append_byte(r, byteval, false);
                    ptr ++;
                    offset ++;
                } else {
                    if (byteval == 0x0D) {
                        suppress = true;
                        ptr ++;
                        offset ++;
                    } else {
                        assert (byteval >= 32 && byteval <= 255);
                    }
                }
            } else {
                raster_append_byte(r, ' ', false);
            }
            if (x > width) {
                //printf("\n");
                x = 0;
                y++;
                suppress = false;
            }
            break;

        case TYPE_COLOR:
            byteval = ptr[0];
            x++;
            color = ptr[1];
            fg = color;
            fg = ( fg & 0x0F ) % 0x0F;
            //printf("fg >= 0x08 = %d\n", fg);

            assert (y < MAX_LINES);
            r = tdc->rasters[y];
            assert(r);
            if (fg >= 0x08) {
                fg -= 0x08;
                /* ANSI control code - hi intensity */
                raster_append_bytes(r, (char *) "\x1b\x5b""1m", 4, false);
            } else {
                /* ANSI control code - normal intensity */
                raster_append_bytes(r, (char *) "\x1b\x5b""21m", 5, false);
            }

            assert(fg >= 0 && fg <= 7);
            bg = color;
            bg = ((bg & 0xF0) >> 4) % 0x08;
            assert(bg >= 0 && bg <= 7);
            fg = ansi_color_map[fg];
            bg = ansi_color_map[bg];

//            printf("[0x%02x][0x%02x] [%02x][%02x] %c\n", byteval, color, bg, fg, byteval);
                if (byteval >= 32) {
                    //printf("^[%u;%um",40 + bg, 30 + fg);
                    snprintf((char *) &ansi_buffer, MAX_ANSI_SEQUENCE, "\x1b\x5b""%u;%um", 40 + bg, 30 + fg);
                        
                    if (!suppress) {
                        raster_append_bytes(r, (char *) &ansi_buffer, strlen(ansi_buffer), false);
                        raster_append_byte(r, byteval, false);
                        }
                    ptr += 2;
                    offset += 2;
                } else {
                    if (byteval == 0x0D) {
                        //printf("<S>");
                        if (!suppress) {
                        /* reset to transparent */
                        raster_append_bytes(r, (char *) "\x1b\x5b""40;37m", 8, false);
                        } 
                        suppress = true;
                        //raster_append_byte(r, ' ', false);
                        ptr ++;
                        offset ++;
                    } else {
                        assert (byteval >= 32 && byteval <= 255);
                    }
                }
            if (x > width) {
                //printf("<H>\n");
                //printf("\n");
                x = 0;
                y++;
                suppress = false;
            }
            break;
        default:
            printf("+ Unhandled font_type = %d?? Not expected to reach here.\n", type);
            return false;
            break;
        }
    }

    y++;
    tdc->height = height;
    tdc->width = width;
    tdc->rendered = true;
    tdc->discovered_height = y+1;
    if (font->parent_tdf->debug_level) {
        printf("\n");
        printf("(%d bytes; %d lines)\n", offset, y);
    }
    return true;

}


const char *get_font_name(TDFHandle *my_tdf, int id)
{

    TDFFont *fontptr = getfont_by_id(my_tdf, id);
    if (!fontptr) {
        return "MISSING FONT";
    }

    return (const char *) fontptr->name;
}


TDFFont* getfont_by_id(TDFHandle *my_tdf, int id)
{
    int icount = 1;
    TDFFont *fontptr = my_tdf->first_font;

    while (fontptr && icount <= my_tdf->fontcount) {
        if (icount == id) {
            return fontptr;
        }
        fontptr = fontptr->next_font;
        icount++;
    }

    return fontptr;
}

bool push_font(TDFHandle *my_tdf, TDFFont *new_font)
{
    TDFFont *last_fontptr = NULL;
    TDFFont *next_fontptr = NULL;
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

TDFCanvas *new_canvas()
{

    int ii = 0;
    TDFCanvas *canvas = NULL;
    canvas = malloc(sizeof(TDFCanvas));
    assert(canvas);
    memset(canvas, 0, sizeof(TDFCanvas));
    return canvas;

}

TDFRaster* canvas_get_raster(TDFCanvas *canvas, int line)
{
    TDFRaster *r = NULL;
    int raster_count = 0;

    //printf("canvas_get_raster(%u)\n", line);
    assert(canvas);
    r = canvas->first_raster;
    if (!r) {
        /* no rasters */
        //printf("no rasters\n");
        return NULL;
    }

    while (r && raster_count < canvas->lines) {
        //    printf("checking raster with index; %d\n", r->index);
        if (raster_count == line) {
            assert(line == r->index);
            return r;
        }
        r = r->next_raster;
        raster_count++;
    }
    //assert (!r->next_raster);
    assert(!r);
    /* not found */
    return NULL;
}

TDFRaster *canvas_add_raster(TDFCanvas *canvas)
{
    TDFRaster *last_raster = NULL;
    TDFRaster *r = NULL;
    int raster_count = 0;
//    printf("  canvas_add_raster()\n");
    assert(canvas);
    r = canvas->first_raster;
    if (!r) {
        /* no rasters */
        canvas->first_raster = create_new_raster();
        canvas->first_raster->index = raster_count;
        canvas->lines ++;
        return canvas->first_raster;
    }

    /* else, subsequent raster */

    while (r->next_raster && raster_count < canvas->lines) {
        last_raster = r;
        r = r->next_raster;
        raster_count++;
    }

    assert(r);
    assert(!r->next_raster);

    r->next_raster = create_new_raster();
    r->next_raster->index = raster_count + 1;
    r->next_raster->next_raster = NULL;
    canvas->lines++;
    return (r->next_raster);

}

bool canvas_output(TDFCanvas *my_canvas)
{
    TDFRaster *r = NULL;
    int ii = 0;
    assert(my_canvas);

    for (ii = 0; ii < my_canvas->lines ; ii++) {
        r = canvas_get_raster(my_canvas, ii);
        assert(r);
        assert(r->data);
        printf("%s\n", r->data);
    }

    return (true);
}

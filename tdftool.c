#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <ctype.h>
#include <libgen.h>
#include "tdf.h"
#include "osdep.h"

#define NO_ANSI_IN_RASTER

static int ansi_color_map[8] = {
    0, 4, 2, 6, 1, 5, 3, 7
    };


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

    /* zero the my_tdf struct as there could be anything there !*/

    memset(&my_tdf, 0, sizeof(my_tdf));

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

            //printf("my_tdf.fontcount = %u\n", my_tdf.fontcount);
            //printf("FSM = 0x%04x\n", font_sequence_marker);
            my_tdf.fontcount++;
            //printf("my_tdf.fontcount = %u\n", my_tdf.fontcount);
            new_font = create_new_font();
            new_font->data = NULL;
            new_font->parent_tdf = &my_tdf;
            new_font->next_font = NULL;
            new_font->average_width = 0;
            new_font->average_height = 0;
            new_font->defined_characters = 0;


            assert(new_font);

            if (font_sequence_marker != 0x55aa00ff) {
                printf("%s: at font %u, expected 0x55aa00ff, but got 0x%08x\n",
                       basename(input_filename),  my_tdf.fontcount, font_sequence_marker);
                exit(1);
            }

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

            if (reserved1) {
                printf("%s: at font %u, reserved1=0x0, but got 0x%08x\n",
                       basename(input_filename),  my_tdf.fontcount, reserved1);
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
                    new_font->characters[ii].rasters[jj]->chardata = NULL;

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
                new_font->characters[ii].prerendered = false;
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
    unsigned char dummy_spacing[30];
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
        memset(&dummy_spacing, 0, 30);
        assert(1 <= tdf->spacing <= 30);
        memset(&dummy_spacing, ' ', tdf->average_width);

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

            assert(raster_append_bytes(dst_raster, (char*) &dummy_spacing, tdf->average_width, false));
        }
        return true;
    }

    memset(&dummy_spacing, 0, 30);
    assert(1 <= tdf->spacing <= 30);
    memset(&dummy_spacing, ' ', tdf->spacing);

    c -= 33;
    assert(c >= 0 && c <= 93);
    tdc = &tdf->characters[c];

    /* make sure character will fit on canvas vertically */

    if (tdc->undefined) {
        /* if the glyph is undefined, just skip it */
        return true;
    }


    assert(!tdc->undefined);
    assert(tdc->prerendered);

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


        if ((!src_raster->chardata || !src_raster->bytes)) {
            /* the glyph is prerendered, and not a space character,
            	 but this particular raster doesn't have any data.
            	 we fill it with dummy spacing data instead */
            //printf("push_glyph: empty raster %u/%u !!\n", ii, tdc->height);
            //exit(1);
            memset(&dummy_spacing, 0, 30);
            assert(1 <= tdc->width <= 30);
            memset(&dummy_spacing, ' ', tdc->width);
            assert(raster_append_bytes(dst_raster, (char*) &dummy_spacing, tdf->average_width, false));

            /* don't forget the font-level spacing as well */
            memset(&dummy_spacing, 0, 30);
            assert(1 <= tdf->spacing <= 30);
            memset(&dummy_spacing, ' ', tdf->spacing);
            assert(raster_append_bytes(dst_raster, (char*) &dummy_spacing, tdf->spacing, false));
            return true;
        }

        assert(src_raster->chardata);
        assert(src_raster->bytes);
        //printf("dst_raster->bytes = %u\n", dst_raster->bytes);
        //printf("src_raster->bytes = %u\n", src_raster->bytes);
        assert(raster_append_bytes(dst_raster, src_raster->chardata, src_raster->bytes, false));
        assert(raster_append_bytes(dst_raster, (char*) &dummy_spacing, tdf->spacing, false));
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
    assert(tdc->prerendered);

    /* get correct character */
    for (ii = 0; ii < tdc->height; ii++) {
        assert (ii < MAX_LINES);
        tdr = tdc->rasters[ii];
        assert(tdr);
        assert(tdr->bytes);
        assert(tdr->chardata);
        printf("%s (%u,%d)\n", tdr->chardata, tdr->bytes, (tdr->bytes ? strlen(tdr->chardata): -1));
    }

    return true;
}


bool raster_append_byte(TDFRaster *r, unsigned char data, bool debug)
{
    TDFRaster *tdr = r;
    unsigned char *raster_realloc = NULL;

    assert(r);

    if (!tdr->bytes) {
        tdr->bytes = 1;
        tdr->chardata = malloc(tdr->bytes+1);
        tdr->chardata[0] = data;
        tdr->chardata[1] = '\0';                            /* NULL terminate, so that the rasters can be printed with C library functions */
        return true;
    } else {
        assert(tdr->chardata);
        assert(tdr->bytes);
        tdr->bytes ++;
        if (debug) {
            //    printf("realloc(), tdr->bytes = %d, address = 0x%08x\n", tdr->bytes+1, tdr->chardata);
        }
        raster_realloc = realloc(tdr->chardata, tdr->bytes+1);
        assert(raster_realloc);
        tdr->chardata = raster_realloc;
        tdr->chardata[tdr->bytes-1] = data;
        tdr->chardata[tdr->bytes] = '\0';
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


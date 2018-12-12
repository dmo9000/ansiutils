#ifndef __MINGW__
#include <arpa/inet.h>
#endif
#include <getopt.h>
#include <ctype.h>
#include <libgen.h>
#include "tdf.h"
#include "ansiraster.h"
#include "tdffont.h"
#include "ansicanvas.h"
#include "osdep.h"

char *strdup(const char *s);
int getopt(int argc, char * const argv[], const char *optstring);

void usage()
{
  printf("\n");
  printf("usage: tdftool [options] <tdf-font> \"your text here\"\n");
  printf("\n");
	printf("    -l              list mode\n");
  printf("    -d              increase debugging level (use up to four times)\n");
  printf("    -v              render vertically instead of horizontally\n");
  printf("    -f <n>          use specified subfont\n");
	printf("    -c              render to CP437 glyphs instead of UTF-8\n");
  printf("    -s              append SAUCE record\n");
  printf("\n");
  return;
}



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
    bool use_unicode = true;
    bool list_mode = false;
    int8_t c = 0;
    char *message = NULL;
    TDFFont *render_font = NULL;
    int debug_level = 0;
    bool vertical = false;
    ANSICanvas *my_canvas = NULL;
    uint16_t running_average_width = 0;
    uint16_t running_average_height = 0;
    bool sauce = false;


    while ((c = getopt (argc, argv, "f:co:dvls")) != -1) {
        switch (c)
        {
        case 's':
            /* append sauce record */
            sauce = true;
            break;
        case 'l':
            list_mode = true;
            break;
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
        case 'c':
            /* disable UNICODE */
            use_unicode = false;
            break;
//        case 'o':
            /* output filename */
            //output_filename = strdup((char *) optarg);
//            break;

        case '?':
            if (optopt == 'c') {
                printf ("Option -%c requires an argument.\n", optopt);
								usage();
                exit(1);
            }
            else if (isprint (optopt)) {
                printf ("Unknown option `-%c'.\n", optopt);
								usage();
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

    /* TODO: a lot of this stuff should be moved into tdffont.c */


    input_filename = (char *) argv[optind];

    optind++;

    if (optind < argc) {
        message = (char *) strdup(argv[optind]);
    }

    /* zero the my_tdf struct as there could be anything there !*/

    memset(&my_tdf, 0, sizeof(my_tdf));

    tdf_file = fopen(input_filename, "rb");


    if (!tdf_file) {
        //printf("Error opening %s\n", input_filename);
				usage();
        exit(1);
    }

    rd = fread((char *) &my_tdf.tdfmagic, TDF_MAGIC_SIZE, 1, tdf_file);
    if (rd != 1) {
        printf("couldn't read %u bytes header from %s - file truncated?\n", TDF_MAGIC_SIZE, input_filename);
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

            if (my_tdf.debug_level > 2) {
                printf("  %2u [%-12s]\n", my_tdf.fontcount, new_font->name);
            }

            if (fread(&reserved1, sizeof(uint32_t), 1, my_tdf.fh) != 1) {
                printf("Failure reading reserved1\n");
                exit(1);
            }

            if (reserved1) {
                printf("%s: at font %u, expected reserved1=0x00000000, but got 0x%08x\n",
                       basename(input_filename),  my_tdf.fontcount, reserved1);
                exit(1);
            }

            assert(!reserved1);

            //printf("reserved1 ok\n");

            if (fread(&new_font->type, sizeof(uint8_t), 1, my_tdf.fh) != 1) {
                printf("couldn't read font type\n");
            }
            assert(new_font->type >= TYPE_OUTLINE);
            assert(new_font->type <= TYPE_COLOR);
            //printf("font type = %u\n", new_font->type);

            if (fread(&new_font->spacing, sizeof(uint8_t), 1, my_tdf.fh) != 1) {
                printf("couldn't read spacing\n");
            }

            assert(new_font->spacing >= 0);
            assert(new_font->spacing <= 40);

            if (my_tdf.debug_level > 2) {
                printf("spacing: %u\n", new_font->spacing);
            }

            if (fread(&new_font->blocksize, sizeof(uint16_t), 1, my_tdf.fh) != 1) {
                printf("couldn't read blocksize\n");
            }

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
                    new_font->characters[ii].char_rasters[jj] = create_new_raster();
                    new_font->characters[ii].char_rasters[jj]->bytes = 0;
                    new_font->characters[ii].char_rasters[jj]->chardata = NULL;
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

    if (list_mode) {
        printf("Font list:\n");
        for (ii = 1; ii <= my_tdf.fontcount; ii++) {
            render_font = getfont_by_id(&my_tdf, ii);
            printf("%d) %s\n", ii, render_font->name);
        }
        exit(0);
    }


    if (message) {
        /* render glyphs */
        if (my_tdf.debug_level > 2) {
            printf("Message to display: %s\n", message);
        }
        render_font = getfont_by_id(&my_tdf, selected_font);
        if (render_font) {
            if (my_tdf.debug_level > 2) {
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
                if (render_font->characters[ii-33].discovered_height > render_font->maximum_height) {
                    render_font->maximum_height = render_font->characters[ii-33].discovered_height;
                }
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

        assert((bool) render_font->defined_characters);
        assert((bool) render_font->average_width);
        assert((bool) render_font->average_height);


        if (vertical) {
            /* FIXME: use the canvas later - this is just to test that rendering is working */
            for (ii = 0; ii < (int) strlen(message) ; ii++)  {
                (void) display_glyph(render_font, (uint8_t) message[ii], use_unicode);
            }
            exit(0);
        }

        my_canvas = new_canvas();
        my_canvas->debug_level = debug_level;


        for (ii = 0; ii < (int) strlen(message); ii++) {
            //printf("+++++ PUSHING GLYPH %u ['%c']\n", ii, message[ii]);
            (void) fflush(NULL);
            assert(push_glyph(my_canvas, render_font, (uint8_t) message[ii]));
        }


        (void) canvas_output(my_canvas, use_unicode, NULL);

    }

    if (fclose(my_tdf.fh) != 0) {
        perror("fclose");
        exit(EXIT_FAILURE);
    };

    if (sauce) {
        int i = 0;
        printf("%cSAUCE", 26);
        for (i = 0 ; i < 123 ; i++) {
            (void) putchar(0x00);
        }
    }

    exit(0);

}








#define _POSIX_C_SOURCE	200112L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include "ansicanvas.h"



bool debug_flag = false;
extern int errno;

#define CONSOLE_WIDTH		    80
#define CONSOLE_HEIGHT	        24

#define CHUNK_SIZE              4096
#define BUF_SIZE 		        8192
#define MAX_ANSI		        64				/* maximum length allowed for an ANSI sequence */
#define MAX_PARAMS	            16


#define SEQ_ERR					0
#define SEQ_NONE				1
#define SEQ_ANSI_1B		    	2
#define SEQ_ANSI_5B				3
#define SEQ_ANSI_IPARAM			4
#define SEQ_ANSI_CMD_M			5
#define SEQ_ANSI_CMD_J			6
#define SEQ_ANSI_CMD_A			7
#define SEQ_ANSI_CMD_B			8
#define SEQ_ANSI_CMD_C			9
#define SEQ_ANSI_CMD_D			10
#define SEQ_ANSI_CMD_H			11
#define SEQ_ANSI_EXECUTED		12
#define SEQ_NOOP			 	13

#define ANSI_1B                 0x1b
#define ANSI_5B                 0x5b
#define ANSI_INTSEP             0x3b

static char *states[] = {
    "SEQ_ERR",
    "SEQ_NONE",
    "SEQ_ANSI_1B",
    "SEQ_ANSI_5B",
    "SEQ_ANSI_IPARAM",
    "SEQ_ANSI_CMD_M",
    "SEQ_ANSI_CMD_J",
    "SEQ_ANSI_CMD_A",
    "SEQ_ANSI_CMD_B",
    "SEQ_ANSI_CMD_C",
    "SEQ_ANSI_CMD_D",
    "SEQ_ANSI_CMD_H",
    "SEQ_ANSI_EXECUTED",
    "SEQ_NOOP"
};

int saved_cursor_x = 0;
int saved_cursor_y = 0;


char ansi_mode = SEQ_NONE;
char last_ansi_mode = SEQ_NONE;
int ansioffset = 0;
int parameters[MAX_PARAMS];
uint8_t paramidx = 0;
uint8_t paramval = 0;
uint8_t paramcount = 0;
off_t offset = 0;
off_t current_escape_address = 0;

uint16_t current_x = 0;
uint16_t current_y = 0;

ansicolor_t fgcolor = 7;
ansicolor_t bgcolor = 0;

#define ATTRIB_NONE				0
#define ATTRIB_REVERSE			1
#define ATTRIB_BLINKING			2
#define ATTRIB_HALFINTENSITY	4
#define ATTRIB_UNDERLINE		8
#define ATTRIB_BOLD				16

uint8_t attributes = 0;

bool auto_line_wrap = false;
bool allow_clear = false;

int ansi_decode_cmd_m();									/* text attributes, foreground and background color handling */
int ansi_decode_cmd_J();									/* "home" command (2J) */
int ansi_decode_cmd_A();									/* move down n lines */
int ansi_decode_cmd_B();									/* move down n lines */
int ansi_decode_cmd_D();									/* move left n columns */
int ansi_decode_cmd_C();									/* move right n columns */
int ansi_decode_cmd_H();									/* set cursor position */

void init_parameters();
const char *ansi_state(int s);

#define FLAG_NONE   0
#define FLAG_1B     1
#define FLAG_5B     2
#define FLAG_INT    4
#define FLAG_ALL    0xFF


uint8_t ansiflags = 0;

void dispatch_ansi_command(ANSICanvas *canvas, unsigned char c);

void init_parameters()
{

    paramidx = 0;
    paramcount = 0;
    paramval = 0;
    for (int i = 0; i < MAX_PARAMS; i++) {
        parameters[i] = 0;
    }

}

void set_ansi_flags(uint8_t flagtype)
{
    ansiflags |= flagtype;

}

void clear_ansi_flags(uint8_t flagtype)
{
    ansiflags &= ~flagtype;
}

void set_ansi_mode(unsigned char mode)
{
    printf("switch mode to [%s]\n", states[mode]);
    last_ansi_mode = ansi_mode;
    ansi_mode = mode;

}


bool send_byte_to_canvas(ANSICanvas *canvas, unsigned char c)
{
    ANSIRaster *r = NULL;
    uint8_t fg = 7, bg = 0;

    //fg = fgcolor + ((attributes & ATTRIB_BOLD) ? 8 : 0);
    fg = fgcolor;
    bg = bgcolor;


    if (current_x >= 80 && auto_line_wrap) {
        if (debug_flag) {
            printf("  * AUTO LINE WRAP at [%u,%u]->[%u,%u]\n", current_x, current_y, current_x - 80, current_y+1);
        }
        current_x -= 80;
        current_y ++;
        if (canvas->scroll_on_output) {
                  assert(current_y < canvas->scroll_limit);
                  }

    }

    r = canvas_get_raster(canvas, current_y);

    while (!r) {
        if (debug_flag) {
            printf("send_byte_to_canvas(%u, %u): line %u does not exist, requesting new\n", current_x, current_y, current_y);
        }
        if (!canvas_add_raster(canvas)) {
            printf("*** error adding raster to canvas (line %u)\n", current_y);
            exit(1);
        };
        r = canvas_get_raster(canvas, current_y);
    }

    if (debug_flag) {
        printf("send_byte_to_canvas(%u, %u): retrieved raster line %u (%u bytes)\n", current_x, current_y, current_y, r->bytes);
    }
    if ((current_x+1) > r->bytes) {
        if (debug_flag) {
            printf("send_byte_canvas(%u, %u): raster is too short (%u bytes), extending\n", current_x, current_y, r->bytes);
        }
        raster_extend_length_to(r, (current_x+1));
    }

    assert(r->bytes >= (current_x+1));

    /* ADJUSTMENT FOR LINEFEED */

    r->chardata[current_x] = c;
    r->fgcolors[current_x] = fg;
    r->bgcolors[current_x] = bg;
    r->attribs[current_x] = attributes;
    current_x++;


    return true;

}

bool ansi_to_canvas(ANSICanvas *canvas, unsigned char *buf, size_t nbytes, size_t offset)
{
    unsigned char c = 0, last_c = 0;
    size_t o = 0;

    /* add an initial raster */
    //canvas_add_raster(canvas);

    while (o < nbytes) {

        /* get next character from stream */
        last_c = c;
        c = buf[o];
        if (debug_flag) {
            printf("[FLAGS=0x%02x(%u,%u)->(0x%08lx)] %lu/%lu/%lu = 0x%02x: ", ansiflags, current_x, current_y, offset+o, o, nbytes, offset+o, c);
        }
        switch(ansiflags) {
        case FLAG_NONE:

            if (c == 26) {
                /* EOF */
                return false;
            }

            if (c == ANSI_1B) {
                if (debug_flag) {
                    printf("ANSI_1B\n");
                }
                set_ansi_flags(FLAG_1B);
            } else {
                if (c == '\r' || c == '\n') {
                    if (debug_flag) {
                        printf("LINE BREAK! (%u)\n", c);
                    }
                    current_x = 0;
                    if (c == '\n') {
                        ANSIRaster *r = NULL;
                        current_y++;
                        if (canvas->scroll_on_output) {
                            assert(current_y < canvas->scroll_limit);
                            }

                        r = canvas_get_raster(canvas, current_y);
                        if (!r) {
                            if (!canvas_add_raster(canvas)) {
                                printf("*** error adding raster to canvas (line %u)\n", current_y);
                                exit(1);
                            };
                            if (debug_flag) {
                                printf("  $$$ canvas now has %u lines\n\n", canvas_get_height(canvas));
                            }
                        }
                    }

                } else {
                    if (debug_flag) {
                        printf("output character '%c'\n", c);
                    }

                    if (canvas->scroll_on_output) {
                        assert(current_y < canvas->scroll_limit);
                        printf("*** CANVAS SCROLL REQUEST ***\n");
                        assert(NULL);
                        }

                    if (!send_byte_to_canvas(canvas, c)) {
                        printf("ERROR writing byte to canvas at (%u, %u)\n", current_x, current_y);
                        exit(1);
                    }
                }
            }
            break;
        case FLAG_1B:
            if (c != ANSI_5B) {
                printf("error: ANSI_5B expected\n");
                /* not sure what to do here */
                clear_ansi_flags(FLAG_ALL);
                break;
            }
            if (debug_flag) {
                printf("ANSI_5B\n");
            }
            set_ansi_flags(FLAG_5B);
            init_parameters();
            break;
        case (FLAG_1B | FLAG_5B):

            if (c == ';') {
                /* parameter with value 0 */
                paramcount ++;
                paramidx ++;
                break;
            }

            if (c == 's') {
                /* save cursor position */
                saved_cursor_x = current_x;
                saved_cursor_y = current_y;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'u') {
                /* restore saved cursor position */
                current_x = saved_cursor_x;
                current_y = saved_cursor_y;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == '?') {
                /* non standard extension! */
                printf( "(! non-standard extension (? ... -> l)\n");
                o++;
                while (c != 'l') {
                    last_c = c;
                    c = buf[o];
                    o++;
                }
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'A') {
                /* if this appears raw, it is probably a mistake, or just padding */
                current_y -= (parameters[0] ? parameters[0] : 1);
                clear_ansi_flags(FLAG_ALL);
                break;
            }



            if (c == 'B') {
                current_y += (parameters[0] ? parameters[0] : 1);
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'C') {
                current_x += (parameters[0] ? parameters[0] : 1);
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'H') {
                /* HOME with 0 parameters */
                printf("  ++ GOT HOME WITH 0 PARAMETERS\n");
                current_x = 0;
                current_y = 0;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'J') {
                assert(!paramidx);
                /* means erase from current line to bottom of screen? currently not implemented */
                printf("[ANSI J command not implemented]\n");
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'K') {
                assert(!paramidx);
                /* means clear to end of current line - not implemented */
                printf("[ANSI K command not implemented]\n");
                clear_ansi_flags(FLAG_ALL);
                break;
                }

            if (isdigit(c)) {
                paramval = c - 0x30;
                if (debug_flag) {
                    printf("start integer parameter [%u], current_value = %u\n", paramidx, paramval);
                }
                set_ansi_flags(FLAG_INT);
            } else {
                printf("error: expecting digit, got '%c' (0x%02x), %u parameter, paramval = %u\n", c, c, paramidx, paramval);
                exit(1);
            }
            break;
        case (FLAG_1B | FLAG_5B | FLAG_INT):
            if (isalpha(c)) {
                parameters[paramidx] = paramval;
                paramcount ++;
                if (debug_flag) {
                    printf("assign integer parameter [%u] = %u\n", paramidx, parameters[paramidx]);
                }
                paramidx++;
                if (debug_flag) {
                    printf("got alphabetical '%c', dispatching\n", c);
                }
                dispatch_ansi_command(canvas, c);
                clear_ansi_flags(FLAG_1B | FLAG_5B | FLAG_INT);
                break;
            }
            if (c == ANSI_INTSEP) {
                /* integer seperator */
                parameters[paramidx] = paramval;
                paramcount ++;
                if (debug_flag) {
                    printf("assign integer parameter [%u] = %u\n", paramidx, parameters[paramidx]);
                }
                paramidx++;
                paramval = 0;
                clear_ansi_flags(FLAG_INT);
            } else {
                if (isdigit(c)) {
                    paramval = (paramval * 10) + (c - 0x30);
                    if (debug_flag) {
                        printf(" cont integer parameter [%u], current_value = %u\n", paramidx, paramval);
                    }
                } else {
                    printf("error: expecting digit or seperator, got '%c' (0x%02x)\n", c, c);
                    exit(1);
                }
            }
            break;
        default:
            printf("unknown flags state = %u\n", ansiflags);
            exit(1);
            break;
        }
        o++;
    }
//    c = 0;
//    last_c = 0;
//    assert(!c);
//    assert(!last_c);
    assert (last_c || !last_c);
    if (debug_flag) {
        printf("BLOCK DONE\n");
    }
    return true;
}

void dispatch_ansi_text_attributes()
{
    if (debug_flag) {
        printf("--- dispatching %u text attribute parameters\n", paramcount);
    }
    for (int i = 0; i < paramcount; i++) {
        if (debug_flag) {
            printf("  + parameter[%u/%u] = %u\n", i, paramcount - 1, parameters[i]);
        }
        if (parameters[i] >= 30 && parameters[i] <= 37) {
            fgcolor = parameters[i] - 30;
            if (debug_flag) {
                printf("  * fgcolor = %u\n", fgcolor);
            }
            goto next_parameter;
        }
        if (parameters[i] >= 40 && parameters[i] <= 47) {
            bgcolor = parameters[i] - 40;
            if (debug_flag) {
                printf("  * bgcolor = %u\n", bgcolor);
            }
            goto next_parameter;
        }

        switch(parameters[i]) {
        case 0:
            if (debug_flag) {
                printf("  * reset all parameters, rg/bg etc.\n");
            }
            fgcolor = 7;
            bgcolor = 0;
            attributes = ATTRIB_NONE;
            goto next_parameter;
            break;
        case 1:
            if (debug_flag) {
                printf("  * enable ATTRIB_BOLD\n");
            }
            attributes |= ATTRIB_BOLD;
            goto next_parameter;
            break;
        case 21:
            if (debug_flag) {
                printf("  * disable ATTRIB_BOLD\n");
            }
            attributes &= ~ATTRIB_BOLD;
            goto next_parameter;
            break;
        default:
            printf("unknown 'm' parameter value: %u\n", parameters[i]);
            exit(1);
            break;
        }

        printf("unknown 'm' parameter value: %u\n", parameters[i]);
        exit(1);

next_parameter:
        ;
    }
}

void dispatch_ansi_terminal_setup()
{
    switch (parameters[0]) {
    case 7:
        /* enable line wrap */
        printf("  * ENABLE LINE WRAP\n");
        auto_line_wrap = true;
        break;
    default:
        printf("  ! UNKNOWN ANSI TERMINAL SETUP CODE\n");
        exit(1);
        break;
    }

    return;
}

void dispatch_ansi_cursor_right(ANSICanvas *canvas)
{
    ANSIRaster *r = NULL;
    uint16_t n = parameters[0];
    uint16_t extend_len = 0;
    if (debug_flag) {
        printf("  > move cursor right %u characters [%u,%u]->[%u,%u]\n", n, current_x, current_y, current_x+n, current_y);
    }
    r = canvas_get_raster(canvas, current_y);
    if (!r) {
        if (!canvas_add_raster(canvas)) {
            printf("   XXX failure adding raster\n");
            exit(1);
        }
    }
    if (debug_flag) {
        printf("current_y = %u\n", current_y);
    }
    r = canvas_get_raster(canvas, current_y);
    assert(r);
    extend_len = current_x + n;
    if (r->bytes < extend_len) {
        if (debug_flag) {
            printf("  > raster is too short (%u), extending to %u\n", r->bytes, extend_len);
        }
        raster_extend_length_to(r, ((extend_len)));
        if (debug_flag) {
            printf("  > raster is now length (%u)\n", r->bytes);
        }
    }
    current_x += n;
    return;

}

void dispatch_ansi_cursor_left(ANSICanvas *canvas)
{
    current_x -= parameters[0];
    return;
}

void canvas_clear(ANSICanvas *canvas)
{
    int ii = 0;
    ANSIRaster *r = NULL;
    r = canvas->first_raster;
    assert(r);
    while (r) {
        assert (r);
        assert (r->bytes);
        assert (r->chardata);
        for (ii = 0; ii < r->bytes; ii++) {
            r->chardata[ii] = ' ';
            r->fgcolors[ii] = 7;
            r->bgcolors[ii] = 0;
            r->attribs[ii] = ATTRIB_NONE;
        }
        r = r->next_raster;
    }
    return;
}

void dispatch_ansi_command(ANSICanvas *canvas, unsigned char c)
{
    if (debug_flag) {
        printf("dispatch_ansi_command('%c')\n", c);
    }

    switch (c) {
    case 'A':
        /* move cursor up parameter[0] rows without changing column */
        current_y-=(parameters[0] ? parameters[0] : 1);
        break;
    case 'B':
        /* move cursor down parameter[0] rows without changing column */
        current_y+=(parameters[0] ? parameters[0] : 1);
        break;
    case 'J':
        /* move home and clear screen - set the clear flag on the canvas if we encounter this */
        if (allow_clear) {
            canvas->clear_flag = true;
        }
        if (canvas->allow_hard_clear) {
            /* blank and dirty the canvas */
            printf("CLEAR SCREEN CALLED, AND HARD CLEAR ENABLED\n");
            canvas_clear(canvas);
            canvas->repaint_entire_canvas = true;
        }
        break;
    case 'C':
        /* move cursor to the right N characters */
        dispatch_ansi_cursor_right(canvas);
        break;
    case 'D':
        /* move cursor to the left N characters */
        dispatch_ansi_cursor_left(canvas);
        break;
    case 'H':
        /* set cursor home - move the cursor to the specified position */
        if (parameters[0] > 0) {
            current_y = parameters[0]-1;
        } else {
            current_y = parameters[0];
        }

        if (parameters[1] > 0) {
            current_x = parameters[1]-1;
        } else {
            current_x = parameters[1];
        }
        break;
    case 'm':
        /* text attributes */
        dispatch_ansi_text_attributes();
        break;
    case 'h':
        /* terminal setup */
        dispatch_ansi_terminal_setup();
        break;
    default:
        printf("+++ unknown ansi command '%c'\n", c);
        exit(1);
        break;
    }

}


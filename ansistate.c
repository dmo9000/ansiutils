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

//#define CONSOLE_WIDTH			80
//#define CONSOLE_HEIGHT		24
#define TABWIDTH 					8

#define CHUNK_SIZE        4096
#define BUF_SIZE 		      8192

/* maximum length allowed for an ANSI sequence */
#define MAX_ANSI		      64
/* maximum number of parameters allowed in ANSI sequence  */
#define MAX_PARAMS				16


#define SEQ_ERR					0
#define SEQ_NONE				1
#define SEQ_ANSI_1B		    	2
#define SEQ_ANSI_5B				3
#define SEQ_ANSI_IPARAM			4
/*
#define SEQ_ANSI_CMD_M					5
#define SEQ_ANSI_CMD_J					6
#define SEQ_ANSI_CMD_A					7
#define SEQ_ANSI_CMD_B					8
#define SEQ_ANSI_CMD_C					9
#define SEQ_ANSI_CMD_D					10
#define SEQ_ANSI_CMD_H					11
#define SEQ_ANSI_EXECUTED				12
#define SEQ_NOOP			 					13
#define SEQ_ANSI_FLAG_CURSOR    14
*/

#define ANSI_1B                 0x1b
#define ANSI_5B                 0x5b
#define ANSI_INTSEP             0x3b
#define ANSI_CURSOR             0x3f
#define ANSI_HASH								0x23

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
    "SEQ_NOOP",
    "SEQ_ANSI_FLAG_CURSOR"
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
uint8_t last_character = 0;

uint16_t current_x = 0;
uint16_t current_y = 0;


ansicolor_t default_fgcolor = 7;
ansicolor_t default_bgcolor = 0;

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

#define FLAG_NONE  		0
#define FLAG_1B    	 	1
#define FLAG_5B    	 	2
#define FLAG_INT   	 	4
#define FLAG_CURSOR	 	8
#define FLAG_HASH			16
#define FLAG_5D				32
#define FLAG_GSTRING	64
#define FLAG_ALL    	0xFF

uint8_t ansiflags = 0;

char *xterm_title_string = NULL;
int xterm_title_string_length = 0;

/* set this to the fd of the parent process if you want to be able to
   respond to DA ("device attribute") requests */

int process_fd = -1;
unsigned char ansi_seqbuf[MAX_ANSI];
uint8_t ansi_offset = 0;
void dispatch_ansi_command(ANSICanvas *canvas, unsigned char c);
int (*ansi_setwindowtitle)(char *s) = NULL;


int ansi_setwindowtitlecallback(int (*setwindowtitle_callback)(char *s))
{
    fprintf(stderr, "ansi_setwindowtitlecallback()\n");
    ansi_setwindowtitle = setwindowtitle_callback;
    return 0;
}

int ansi_setdebug(bool debugstate)
{
    debug_flag = debugstate;

}


void ansi_debug_dump()
{
    int i = 0;
    printf("ansi_debug_dump():\n");
    printf("ansi_offset = %u\n", ansi_offset);
    printf("paramidx = %u\n", paramidx);
    printf("\n");

    printf("Cursor Position: %d,%d\n\n", current_x, current_y);
    printf("Flags:\n");

    if (!ansiflags) {
        printf("FLAG_NONE\n");
    }

    if (ansiflags & FLAG_1B) {
        printf("  FLAG_1B\n");
    }
    if (ansiflags & FLAG_5B) {
        printf("  FLAG_5B\n");
    }
    if (ansiflags & FLAG_INT) {
        printf("  FLAG_INT\n");
    }
    if (ansiflags & FLAG_CURSOR) {
        printf("  FLAG_CURSOR\n");
    }

    if (ansiflags & FLAG_HASH) {
        printf("  FLAG_HASH\n");
    }

    if (ansiflags & FLAG_5D) {
        printf("  FLAG_5D\n");
    }

    if (ansiflags & FLAG_GSTRING) {
        printf("  FLAG_GSTRING\n");
    }

    printf("Parameters:\n\n");

    for (i = 0; i < paramidx; i++) {
        printf("parameter[%u] = %u\n", i, parameters[i]);
    }
    printf("\n\n");

    printf("Sequence:\n  ");

    for (i = 0; i < ansi_offset; i++) {
        if (ansi_seqbuf[i] == 0x1B) {
            printf("<ESC> ");
        } else {
            if (ansi_seqbuf[i] >= 128) {
                printf("{%02X} ", ansi_seqbuf[i]);
            } else {
                printf("%c ",  ansi_seqbuf[i]);
            }
        }
    }
    printf("\n  ");
    for (i = 0; i < ansi_offset; i++) {
        printf("%02X ", ansi_seqbuf[i]);
    }
    printf("\n  echo -ne '");
    for (i = 0; i < ansi_offset; i++) {
        printf("\\x%02x", ansi_seqbuf[i]);
    }
    printf("''FOO'\n\n");


    fflush(NULL);
    exit(1);

}

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

    /* perhaps these should be moved to the tty, since they are not
    		ANSI, but terminal control codes */

    if (c == 7) {
        /* BEL - currently not implemented. Hook it up to a SID emulator
        		perhaps? :^) */
        //printf("[TERMINAL BEL]\n");
        return true;
    }

    if (c == 9) {
        /* TAB */
        //printf("[TAB DETECTED]\n");
        current_x = current_x + TABWIDTH;
        current_x = (current_x / TABWIDTH) * TABWIDTH;
        return true;
    }


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
        //printf("send_byte_to_canvas(%u, %u): retrieved raster line %u (%u bytes)\n", current_x, current_y, current_y, r->bytes);
    }
    if ((current_x+1) > r->bytes) {
        if (debug_flag) {
            printf("send_byte_canvas(%u, %u): raster is too short (%u bytes), extending\n", current_x, current_y, r->bytes);
        }
        raster_extend_length_to(r, (current_x+1));
    }

    if (!(r->bytes >= (current_x+1))) {
        printf("current_x = %d\n", current_x);
        printf("current_y = %d\n", current_y);
    };

    assert(r->bytes >= (current_x+1));

    /* ADJUSTMENT FOR LINEFEED */

    if (debug_flag) {
        printf(">> [%c] {%u,%u} %u,%u [%c]\r\n", c,  current_x, current_y, fg, bg, (attributes & ATTRIB_REVERSE ? 'R' : ' '));
    }

    r->chardata[current_x] = c;
    r->fgcolors[current_x] = fg;
    r->bgcolors[current_x] = bg;
    r->attribs[current_x] = attributes;
    current_x++;


    return true;

}

bool ansi_to_canvas(ANSICanvas *canvas, unsigned char *buf, size_t nbytes, size_t offset)
{
    ANSIRaster *r = NULL;
    int ii = 0, jj = 0;
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
                /* ansi_offset should be 0 when we receive this */
                ansi_offset = 0;
                //assert(!ansi_offset);
                ansi_seqbuf[ansi_offset] = 0x1B;
                ansi_offset++;

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
                    /*
                                    if (debug_flag) {
                                        printf("output character '%c' [%c]\n", c,  (attributes & ATTRIB_REVERSE ? 'R' : ' '));
                                    }
                    */
                    last_character = c;

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
            ansi_seqbuf[ansi_offset] = c;
            ansi_offset++;

            if (c == '=') {
                fprintf(stderr, "+++ ESC =                   (V)     Application Keypad Mode\n");
                /* not sure what to do with this */
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == ']') {
                /*
                	ESC ] 0 ; string ^G     (A)
                	Operating System Command (Hardstatus, xterm title hack) */
                fprintf(stderr, "+++ Operating System Command - ^]\n");
                set_ansi_flags(FLAG_5D);
                last_character = c;
                break;
            }

            if (c == 'E') {
                // next line? current_y++, current_x = 0
                fprintf(stderr, "+++ ^E - ???\n");
                //ansi_debug_dump();
                current_x = 0;
                current_y ++;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'M') {
                // reverse index mode - set current_x = 0?
                fprintf(stderr, "+++ 1B->M (Reverse Index) command\n");
                //ansi_debug_dump();
                //current_x = 0;
                current_y --;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'D') {
                // index mode - offset current_y + 1, but leave current_x untouched?
                current_y ++;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == ANSI_HASH) {
                fprintf(stderr, "+++ WARNING: ANSI_HASH received %d (0x%02x) -> '%c'\n", c, c, c);
                set_ansi_flags(FLAG_HASH);
                break;
            }


            if (c != ANSI_5B) {
                fprintf(stderr, "error: ANSI_5B expected, received %d (0x%02x) -> '%c'\n", c, c, c);
                ansi_debug_dump();
                assert(NULL);
                /* not sure what to do here */
                //clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (debug_flag) {
                printf("ANSI_5B\n");
            }
            //ansi_seqbuf[ansi_offset] = 0x5B;
            //ansi_offset++;

            set_ansi_flags(FLAG_5B);
            init_parameters();
            break;

        case (FLAG_1B | FLAG_5D | FLAG_GSTRING):
            ansi_seqbuf[ansi_offset] = c;
            ansi_offset++;

            switch (c) {
            case 0x07:
                fprintf(stderr, "+++ GSTRING TERMINATED -> {%s}\n", xterm_title_string);
                /* TODO: dispatch to a title bar handler that can set the title */
                if (ansi_setwindowtitle == NULL) {
                    fprintf(stderr, "+++ No callback configured for ansi_setwindowtitle(). Ignoring.\n");
                } else {
                    fprintf(stderr, "+++ -> ansi_setwindowtitle(%s)\n", xterm_title_string);
                    ansi_setwindowtitle(xterm_title_string);
                }


                clear_ansi_flags(FLAG_ALL);
                break;
            default:
                if (c < 32 || c > 128) {
                    fprintf(stderr, "+++ GSTRING range error\n");
                    ansi_debug_dump();
                }
                assert(xterm_title_string_length < 64);
                xterm_title_string_length++;
                xterm_title_string = realloc(xterm_title_string, xterm_title_string_length+1);
                xterm_title_string[xterm_title_string_length-1] = c;
                xterm_title_string[xterm_title_string_length] = '\0';
                break;
            }

            break;

        case (FLAG_1B | FLAG_5D):
            ansi_seqbuf[ansi_offset] = c;
            ansi_offset++;

            switch(c) {
            case '0':
                fprintf(stderr, "+++ Switch to ^G terminated string mode... id [0]\n");
                assert(last_character == ']');
                last_character = c;
                break;
            case ';':
                fprintf(stderr, "+++ Switch to ^G terminated string mode... id [;]\n");
                assert(last_character == '0');
                set_ansi_flags(FLAG_GSTRING);
                if (xterm_title_string != NULL) {
                    free(xterm_title_string);
                    xterm_title_string = NULL;
                    xterm_title_string_length = 0;
                }
                break;
            default:
                fprintf(stderr, "+++ Unrecognized Operating System Command Sequence ('%c'). Aborting.\n", c);
                ansi_debug_dump();
                break;
            }
            break;
        case (FLAG_1B | FLAG_5B):

            /* FIXME: starting to see a lot of duplication here, of commands with no parameters vs
            		commands with parameters. need to take a look whether we can rationalize this without breaking
            		too much */


            if (c == 'r') {
                /* see http://www2.gar.no/glinkj/help/cmds/ansa.htm */
                /* SCR - scrolling region */
                fprintf(stderr, "SCR	Scrolling region	esc [ r	1B 5B 72\n");
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'm') {
                assert(!paramcount);
                assert(!paramidx);
                assert(!parameters[0]);
                //printf("*** raw m command - reset? ***\n");
                fgcolor = 7;
                bgcolor = 0;
                attributes = ATTRIB_NONE;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == ';') {
                ansi_seqbuf[ansi_offset] = ';';
                ansi_offset++;
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
                /* set cursor visible or non-visible */
                set_ansi_flags(FLAG_1B | FLAG_5B | FLAG_CURSOR);
                //printf("cursor handling code encountered\n");
                ansi_seqbuf[ansi_offset] = '?';
                ansi_offset++;
                break;
            }

            if (c == 'A') {
                fprintf(stderr, "+++ please rationalize! %s,%u\n",
                        __FILE__, __LINE__);
                //ansi_debug_dump();
                /* if this appears raw, it is probably a mistake, or just padding */
                current_y -= (parameters[0] ? parameters[0] : 1);
                clear_ansi_flags(FLAG_ALL);
                break;
            }


            if (c == 'B') {
                fprintf(stderr, "+++ please rationalize! %s,%u\n",
                        __FILE__, __LINE__);
                //ansi_debug_dump();
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
                /* FIXME: unify with the parameterized version */
                current_x = 0;
                current_y = 0;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'J') {
                assert(!paramidx);
                /* means erase from current line to bottom of screen? currently not implemented */
                //printf("[ANSI J command not implemented]\n");
                clear_ansi_flags(FLAG_ALL);
                int jj = current_y;
                while (jj < canvas->lines) {
                    r = canvas_get_raster(canvas, jj);
                    assert(r);
                    for (int ii = 0; ii <= r->bytes; ii++) {
                        r->chardata[ii] = ' ';
                        r->fgcolors[ii] = 7;
                        r->bgcolors[ii] = 0;
                        r->attribs[ii] = ATTRIB_NONE;
                    }
                    jj++;
                }
                canvas->repaint_entire_canvas = true;
                break;
            }

            if (c == 'L') {
                /* TODO: move the code below to a canvas_insert_raster() function */
                /* insert a line */
                ANSIRaster *i = NULL;       /* new raster to be inserted */
                ANSIRaster *p = NULL;       /* delete pointer */
                ANSIRaster *l = NULL;       /* pointer to previous raster in list */
                /* insert raster */
                printf("insert raster ...\n");
                assert(!paramidx);
                assert(canvas->default_raster_length);
                i = create_new_raster();
                assert(i);
                raster_extend_length_to(i, canvas->default_raster_length);
                assert(i->bytes == canvas->default_raster_length);
                assert(current_y > 0);
                r = canvas_get_raster(canvas, current_y-1);
                i->next_raster = r->next_raster;
                r->next_raster = i;
                p = i;
                while (p->next_raster != NULL) {
                    l = p;
                    p = p->next_raster;
                }
                assert(!canvas_reindex(canvas));
                printf("reached last raster, index = %u\n", p->index);
                assert(p->next_raster == NULL);
                assert(l->next_raster == p);
                assert(l->index + 1 == p->index);
                /* remove from list */
                l->next_raster = NULL;
                /* reindex again, to ensure line count is correct */
                assert(!canvas_reindex(canvas));
                canvas->repaint_entire_canvas = true;
                canvas->is_dirty = true;
                clear_ansi_flags(FLAG_ALL);
                break;
            }


            if (c == 'M') {
                ANSIRaster *d = NULL;
                ANSIRaster *p = NULL;
                ANSIRaster *n = NULL;
                //            printf("delete raster\n");
                assert(!paramidx);
                p = canvas_get_raster(canvas, current_y-1);
                d = p->next_raster;
                p->next_raster = d->next_raster;
                raster_delete(d);
                canvas_reindex(canvas);
                n = canvas_add_raster(canvas);
                raster_extend_length_to(n, canvas->default_raster_length);
                canvas_reindex(canvas);
                canvas->repaint_entire_canvas = true;
                canvas->is_dirty = true;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'D') {
                /* rubout/delete? */

                if (debug_flag) {
                    fprintf(stderr, "[RUBOUT/DELETE]\n");
                }
                if (current_x ) {
                    current_x --;
                } else {
                    current_y --;
                    current_x = (CONSOLE_WIDTH-1);
                }
                canvas->is_dirty = true;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (c == 'd') {
                /* SSR - SPACE SUPPRESS RESET  */
                fprintf(stderr, "[SSR]\n");
                current_y = 0;
                canvas->is_dirty = true;
                clear_ansi_flags(FLAG_ALL);
                break;
            }

            if (isdigit(c)) {
                ansi_seqbuf[ansi_offset] = c;
                ansi_offset++;
                paramval = c - 0x30;
                if (debug_flag) {
                    printf("start integer parameter [%u], current_value = %u\n", paramidx, paramval);
                }
                set_ansi_flags(FLAG_INT);
            } else {
                ansi_seqbuf[ansi_offset] = c;
                ansi_offset++;
                printf("error: expecting digit, got '%c' (0x%02x), %u parameter, paramval = %u\n", c, c, paramidx, paramval);
                dispatch_ansi_command(canvas, c);
                //ansi_debug_dump();
                //assert(NULL);
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
                    printf("got alphabetical '%c', dispatching with parameter count %d\n", c, paramidx);
                }
                dispatch_ansi_command(canvas, c);
                clear_ansi_flags(FLAG_1B | FLAG_5B | FLAG_INT);
                break;
            }
            if (c == ANSI_INTSEP) {
                /* integer seperator */
                ansi_seqbuf[ansi_offset] = c;
                ansi_offset++;
                parameters[paramidx] = paramval;
                paramcount ++;
                if (debug_flag) {
                    fprintf(stderr, "assign integer parameter [%u] = %u\n", paramidx, parameters[paramidx]);
                }
                paramidx++;
                paramval = 0;
                clear_ansi_flags(FLAG_INT);
            } else {
                if (isdigit(c)) {
                    ansi_seqbuf[ansi_offset] = c;
                    ansi_offset++;
                    paramval = (paramval * 10) + (c - 0x30);
                    if (debug_flag) {
                        fprintf(stderr, " cont integer parameter [%u], current_value = %u\n", paramidx, paramval);
                    }

                    if (paramval > 255) {
                        fprintf(stderr, " +++ integer paramater out of range? (paramval = %u)\n", paramval);
                        ansi_debug_dump();
                        exit(1);
                    }

                } else {
                    printf("error: expecting digit or seperator, got '%c' (0x%02x)\n", c, c);
                    printf("paramval = %u\n", paramval);
                    clear_ansi_flags(FLAG_ALL);
                    break;
                    //assert(NULL);
                }
            }
            break;

        case (FLAG_1B | FLAG_HASH):
            /* hash mode - what mysteries await! */
            ansi_seqbuf[ansi_offset] = c;
            ansi_offset++;

            switch (c) {
            case '8':
                /* vttest describes this as decaln() -
                							"Screen Alignment Display" */

                if (debug_flag) {
                    fprintf(stderr, "+++ ANSI_HASH mode: fill screen with E's\n");
                }
                for (jj = 0; jj < canvas_get_height(canvas); jj++) {
                    r = canvas_get_raster(canvas, jj);
                    assert(r);
                    for (ii = 0; ii < r->bytes; ii++) {
                        r->chardata[ii] = 'E';
                    }
                }
                /* gnome-terminal and xterm both put cursor in top
                	 left hand corner, so will we */

                current_x = 0;
                current_y = 0;
                canvas->repaint_entire_canvas = true;
                canvas->is_dirty = true;
                break;
            default:
                fprintf(stderr, "+++ ANSI_HASH mode: unknown/unimplemented\n");
                ansi_debug_dump();
                assert(NULL);
                break;
            }
            clear_ansi_flags(FLAG_ALL);
            break;

        case (FLAG_1B | FLAG_5B | FLAG_CURSOR):
            /* cursor visibility state */
            //printf("in cursor visibility state!\n");
            switch(c) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                /* in number sequence */
                ansi_seqbuf[ansi_offset] = c;
                ansi_offset++;
                paramval = (paramval * 10) + (c - 0x30);
                break;
            case 'h':
                /* see:
                	 http://ascii-table.com/ansi-escape-sequences-vt-100.php
                */
                ansi_seqbuf[ansi_offset] = 'h';
                ansi_offset++;

                switch(paramval) {
                case 1:
                    /* not sure what to do with this */
                    fprintf(stderr, "+++ ESC[?1h 'Application Cursor Keys'");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 3:
                    /* set to 132 column mode */
                    fprintf(stderr, "+++ ^[?3h - 132 column mode!\n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 7:
                    fprintf(stderr, "Esc[?7h 	Set auto-wrap mode 	DECAWM \n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 8:
                    fprintf(stderr, "+++ enable auto-repeat mode?? vttest\n");
                    /* not implemented, how would we turn it off? */
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 25:
                    printf("*** hide cursor\n");
                    canvas->cursor_enabled = false;
                    canvas->is_dirty=true;
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 40:
                    /* no idea - https://psi-matrix.eu/wordpress/wp-content/uploads/2016/08/Programmers-Guide-DEC-LA324-1.pdf ? */
                    /* CR acts as new line?? */
                    fprintf(stderr, "[UNKNOWN - CR acts as new line?]\n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                default:
                    fprintf(stderr, "+++ unimplemented ?<n>h SET_MODE code\n");
                    fprintf(stderr, "+++ received <nn>h, <nn> was %u\n", paramval);
                    ansi_debug_dump();
                    break;
                }
                clear_ansi_flags(FLAG_ALL);
                break;
            case 'l':
                /* see:
                	 http://ascii-table.com/ansi-escape-sequences-vt-100.php
                */

                ansi_seqbuf[ansi_offset] = 'l';
                ansi_offset++;

                switch(paramval) {
                case 1:
                    fprintf(stderr, "Esc[?1l 	Set cursor key to cursor 	DECCKM \n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 3:
                    fprintf(stderr, "Esc[?3l 	Set number of columns to 80 	DECCOLM\n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 4:
                    fprintf(stderr, "Esc[?4l 	Set jump scrolling 	DECSCLM\n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 5:
                    fprintf(stderr, "Esc[?5l 	Set normal video on screen 	DECSCNM \n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 6:
                    fprintf(stderr, "Esc[?6l 	Set origin to absolute 	DECOM\n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 8:
                    fprintf(stderr, "Esc[?8l 	Reset auto-repeat mode 	DECARM \n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 25:
                    /* show cursor */
                    printf("*** show cursor\n");
                    canvas->cursor_enabled = true;
                    canvas->is_dirty=true;
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 33:
                    /* a non-standard extension. See README.notes for a list of potential users of this */
                    fprintf(stderr, "{+++ non standard sequence CSI [ ? 33 l}\n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                case 45:
                    /* no idea - vttest sends this ? */
                    fprintf(stderr, "[UNKNOWN - vttest - FIXME]\n");
                    clear_ansi_flags(FLAG_ALL);
                    break;
                default:
                    fprintf(stderr, "+++ unimplemented ?<n>l RESET_MODE code\n");
                    fprintf(stderr, "+++ received <nn>l, <nn> was %u\n", paramval);
                    ansi_debug_dump();
                    break;
                }
                clear_ansi_flags(FLAG_ALL);
                break;
            default:
                printf("unknown cursor visibility command (%c), paramval = %u\n", c, paramval);
                assert(NULL);
                break;
            }
            break;
        default:
            printf("unknown flags state = %u\n", ansiflags);
            exit(1);
            break;
        }
        o++;
    }
fallback_exit:
    assert (last_c || !last_c);
//if (debug_flag) {
//printf("BLOCK DONE\n");
//}
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
        case 2:
            if (debug_flag) {
                printf("  * enable ATTRIB_HALFINTENSITY\n");
            }
            printf("[ATTRIB_HALFINTENSITY ON]\n");
            attributes |= ATTRIB_HALFINTENSITY;
            goto next_parameter;
            break;
        case 4:
            if (debug_flag) {
                printf("  * enable ATTRIB_UNDERLINE\n");
            }
            attributes |= ATTRIB_UNDERLINE;
            goto next_parameter;
            break;
        case 5:
            if (debug_flag) {
                printf("  * enable ATTRIB_BLINKING\n");
            }
            printf("[ATTRIB_BLINKING ON]\n");
            attributes |= ATTRIB_BLINKING;
            goto next_parameter;
            break;
        case 7:
            if (debug_flag) {
                printf("  * enable ATTRIB_REVERSE\n");
            }
            attributes |= ATTRIB_REVERSE;
            goto next_parameter;
            break;
        case 10:
            /* primary - default font - currently no effect */
            //printf("[DEFAULT FONT:10m]\n");
            goto next_parameter;
            break;
        case 21:
            if (debug_flag) {
                printf("  * disable ATTRIB_BOLD\n");
            }
            attributes &= ~ATTRIB_BOLD;
            goto next_parameter;
            break;
        case 22:
            if (debug_flag) {
                printf("  * disable ATTRIB_HALFINTENSITY\n");
            }
            printf("[ATTRIB_HALFINTENSITY OFF]\n");
            attributes &= ~ATTRIB_HALFINTENSITY;
            goto next_parameter;
            break;
        case 24:
            if (debug_flag) {
                printf("  * disable ATTRIB_UNDERLINE\n");
            }
            attributes &= ~ATTRIB_UNDERLINE;
            goto next_parameter;
            break;
        case 25:
            if (debug_flag) {
                printf("  * disable ATTRIB_BLINKING\n");
            }
            printf("[ATTRIB_BLINKING OFF]\n");
            attributes &= ~ATTRIB_BLINKING;
            goto next_parameter;
            break;
        case 27:
            if (debug_flag) {
                printf("  * disable ATTRIB_REVERSE\n");
            }
            printf("[ATTRIB_REVERSE OFF]\n");
            attributes &= ~ATTRIB_REVERSE;
            goto next_parameter;
            break;
        case 38:
            /* seems to do nothing, but coreutils color ls send it to an xterm? */
            fprintf(stderr, "+++ unknown ^[38m sequence!\n");
            goto next_parameter;
            break;
        case 39:
            /* default foreground color - currently not implemented*/
            //printf("[39m:DEFAULT FOREGROUND COLOR::NOT IMPLEMENTED YET]\n");
            fgcolor = default_fgcolor;
            goto next_parameter;
            break;
        case 49:
            /* default background color - currently not implemented*/
            //printf("[39m:DEFAULT BACKGROUND COLOR::NOT IMPLEMENTED YET]\n");
            bgcolor = default_bgcolor;
            goto next_parameter;
            break;
        default:
            printf("+++ unknown 'm' parameter value: %u (paramidx=%u, ansiflags=%u)\n", parameters[i], paramidx, ansiflags);
            ansi_debug_dump();
            assert(NULL);
            break;
        }

        printf("+++unknown 'm' parameter value: %u (paramidx=%u, ansiflags=%u)\n", parameters[i], paramidx, ansiflags);
        ansi_debug_dump();
        assert(NULL);

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

    /* there is a trick to this one - the parameter can only have a minimum
    		value of 1, even if a parameter is passed which is zero, or
    		no parameter is passed. The minimum number of spaces the cursor
    		can move to the right is 1 */

    if (!n) {
        n = 1;
    }

    fprintf(stderr, "+++ ^[%uC (dispatch_ansi_cursor_right)\n", parameters[0]);
    fprintf(stderr, "parameters[0]=%u\n", parameters[0]);
    fprintf(stderr, "current_x=%u\n", current_x);

    /* clamp to right hand side of canvas */
    /* FIXME: this needs to be able to work with a resizable terminal ... */
    /* FIXME: this triggers a wrapping bug ... */

    if (current_x + n > (CONSOLE_WIDTH - 1)) {
        n = (CONSOLE_WIDTH - 1) - current_x;
    }

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

    /* this needs to be clamped to the screen edge */

    current_x += n;

    fprintf(stderr, "+++ position = %d,%d\n", current_x, current_y);
    return;

}

void dispatch_ansi_cursor_left(ANSICanvas *canvas)
{
    fprintf(stderr, "+++ ^[%dD (dispatch_ansi_cursor_left)\n", parameters[0]);
    fprintf(stderr, "parameters[0]=%u\n", parameters[0]);
    fprintf(stderr, "current_x=%u\n", current_x);
    if ((parameters[0] ? parameters[0] : 1) > current_x) {
        /* clamp to left hand edge */
        /* careful here - comparison between signed/unsigned? */
        current_x = 0;
    } else {
        //current_x -= parameters[0];
        /* must always move at least one position */
        current_x -= (parameters[0] ? parameters[0] : 1);
    }
    fprintf(stderr, "+++ position = %d,%d\n", current_x, current_y);
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

    /* this should be called 'dispatch_ansi_command_with_parameter' - in fact, parameterized vs non-parameterized
    	 implementations need to be merged to prevent too much duplication of code */
    char response[2048];
    ANSIRaster *r = NULL;				/* general purpose raster */
    ANSIRaster *d = NULL;
    ANSIRaster *p = NULL;
    ANSIRaster *n = NULL;
    int i = 0;
    int ii = 0;
    int jj = 0;
    uint8_t *repeat_char = NULL;
    if (debug_flag) {
        printf("dispatch_ansi_command('%c')\n", c);
    }

    ansi_seqbuf[ansi_offset] = c;
    ansi_offset++;

    switch (c) {
    case 'A':
        fprintf(stderr, "+++ ^[ %dA - move cursor up %d rows\n",
                parameters[0], (parameters[0] ? parameters[0] : 1));
        /* move cursor up parameter[0] rows without changing column */
        if (current_y >= (parameters[0] ? parameters[0] : 1)) {
            current_y-=(parameters[0] ? parameters[0] : 1);
        } else {
            /* clamp to top of screen */
            current_y = 0;
        }
        break;
    case 'b':
        /* testing - probably wrong since it's supposed to be able to repeat the last control sequence as well */
        //fprintf(stderr, "[REP  Repeat Char or Control    Esc [ Pn b                   1]\n");
        //fprintf(stderr, "last_character=%u (0x%2x), repeat=%d\n",  last_character, last_character, parameters[0]);
        //ansi_debug_dump();
        assert(parameters[0]);
        repeat_char = malloc(parameters[0]);
        for (i = 0 ; i < parameters[0]; i++) {
            repeat_char[i] = last_character;
            //send_byte_to_canvas(canvas, last_character);
        }
        clear_ansi_flags(FLAG_ALL);
        ansi_to_canvas(canvas, repeat_char, parameters[0], 0);
        canvas->repaint_entire_canvas = true;
        canvas->is_dirty = true;
        free(repeat_char);
        break;
    case 'B':
        fprintf(stderr, "+++ ^[ %dB - move cursor down %d rows\n",
                parameters[0], (parameters[0] ? parameters[0] : 1));
        /* move cursor down parameter[0] rows without changing column */
        current_y+=(parameters[0] ? parameters[0] : 1);
        /* clamp to bottom of screen/canvas etc... */
        if (current_y > ( CONSOLE_HEIGHT -1) ) {
            current_y = CONSOLE_HEIGHT -1 ;
        }
        break;
    case 'G':
        /* unix mode reset? not implemented! */
        fprintf(stderr, "[1B 5B 62:UMR - UNIX MODE RESET? NOT IMPLEMENTED!]\n");
        break;
    case 'C':
        /* move cursor to the right N characters */
        dispatch_ansi_cursor_right(canvas);
        break;
    case 'd':
        ansi_seqbuf[ansi_offset] = 'd';
        ansi_offset++;
        /* testing - space supress reset? */
        //printf("[1B 5B <nn> 64:VPA - VERTICAL POSITION ABSOLUTE:Y=%u]\n", parameters[0]);
        /* leave x position where it is, but move y to line 0 */
        assert((parameters[0] - 1) >= 0);
        current_y = parameters[0] - 1;
        clear_ansi_flags(FLAG_ALL);
        canvas->repaint_entire_canvas = true;
        canvas->is_dirty= true;
        break;
    case 'D':
        dispatch_ansi_cursor_left(canvas);
        break;
    case 'f':
    /* direct cursor addressing  - same as 'H' */
    case 'H':
        /* set cursor home - move the cursor to the specified position */
        //    if (debug_flag) {
        fprintf(stderr, "+++ SET CURSOR HOME(%u, %u)\n", parameters[1], parameters[0]);
        // }

        if (debug_flag) {
            fprintf(stderr, " 1) set_cursor_home(%u,%u)\n", current_x, current_y);
        }

        if (parameters[1] > 0) {
            current_x = parameters[1] - 1;
        } else {
            current_x = 0;
        }

        if (parameters[0] > 0) {
            current_y = parameters[0] - 1;
        } else {
            current_y = 0;
        }

        if (debug_flag) {
            fprintf(stderr, " 2) set_cursor_home(%u,%u)\n", current_x, current_y);
        }

        /* very dodgy - hardcoded is bad. we might want to find some other way to calculate these,
        	 especially if a custom canvas size is in use */

        if (current_x > CONSOLE_WIDTH -1) {
            current_x = CONSOLE_WIDTH -1;
        }

        if (current_y > CONSOLE_HEIGHT -1) {
            current_y = CONSOLE_HEIGHT -1;
        }

//        if (debug_flag) {
        fprintf(stderr, " 3) set_cursor_home(%u,%u)\n", current_x, current_y);
//        }

        break;
    case 'M':
        /* see: http://www2.gar.no/glinkj/help/cmds/vipa.htm */
        /* delete line - UE4 prototype has implementation of this*/
        //fprintf(stderr, "[DELETE LINES %u:%d]\n", current_y, parameters[0]);
        //fprintf(stderr, "default raster length = %u\n", canvas->default_raster_length);
        assert(paramidx == 1);
        for (i = 0; i < parameters[0]; i++) {
            /* add raster to end */
            n = canvas_add_raster(canvas);
            raster_extend_length_to(n, canvas->default_raster_length);
            canvas_reindex(canvas);
        }
        for (i = 0; i < parameters[0]; i++) {
            //printf("delete raster %u\n", i);
            p = canvas_get_raster(canvas, current_y-1);
            d = p->next_raster;
            p->next_raster = d->next_raster;
            raster_delete(d);
            canvas_reindex(canvas);
        }
        canvas_reindex(canvas);
        canvas->repaint_entire_canvas = true;
        canvas->is_dirty = true;
        clear_ansi_flags(FLAG_ALL);
        break;
    case 'm':
        /* text attributes */
        dispatch_ansi_text_attributes();
        break;
    case 'h':
        /* terminal setup */
        dispatch_ansi_terminal_setup();
        break;
    case 'c':
        /* device attributes */
        if (debug_flag) {
            fprintf(stderr, "+++ { DA	Device attributes	esc [ c	1B 5B 63 }\n");
        }
        if (process_fd != -1) {
            printf("(responding to DA on fd %d)\n", process_fd);
            write(process_fd, "\x1b\x5b""?62;c", 7);
            return;
        }
        assert(process_fd == -1);
        break;
    case 'n':
        /* apparently this is "report cursor position". At least vim uses this  */
        snprintf(&response, 256, "%d;%dR", current_y+1, current_x+1);
        /* FIXME: error checking */
        write(process_fd, "\x1b\x5b", 2);
        write(process_fd, response, strlen(response));
        clear_ansi_flags(FLAG_ALL);
        break;
    case 'J':
        /* https://www.gnu.org/software/screen/manual/html_node/Control-Sequences.html
        		ESC [ Pn J                      Erase in Display
         		Pn = None or 0            From Cursor to End of Screen
              1                    From Beginning of Screen to Cursor
              2                    Entire Screen
           */
        if (paramidx > 1) {
            fprintf(stderr, "+++ 'J' command -- too many parameters!\n");
            ansi_debug_dump();
        }
        if (!paramidx) {
            fprintf(stderr, "+++ '0J' command -- erase from cursor to end of screen\n");
            ansi_debug_dump();
        }

        switch (parameters[0]) {
        case 0:
            fprintf(stderr, "+++ '0J' command -- erase from cursor to end of screen (last row=%d)\n", canvas_get_height(canvas));
            /* strictly speaking, this deletes from the line containing
            	 the current cursor */
            for (jj = current_y; jj < canvas_get_height(canvas) ; jj++) {
                r = canvas_get_raster(canvas, jj);
                assert(r);
                for (ii = 0; ii < r->bytes; ii++) {
                    r->chardata[ii] = ' ';
                }
            }
            canvas->repaint_entire_canvas = true;
            break;
        case 1:
            fprintf(stderr, "+++ '1J' command -- erase from beginning of screen to cursor(%d,%d)\n", current_x, current_y);
            for (jj = 0; jj < current_y ; jj++) {
                r = canvas_get_raster(canvas, jj);
                assert(r);
                for (ii = 0; ii < r->bytes; ii++) {
                    r->chardata[ii] = ' ';
                }
            }
            r = canvas_get_raster(canvas, current_y);
            assert(r);
            for (ii = 0; ii < current_x; ii++) {
                r->chardata[ii] = ' ';
            }
            canvas->repaint_entire_canvas = true;
            break;
        case 2:
            fprintf(stderr, "+++ 'J' command -- erase entire screen\n");
            if (allow_clear) {
                canvas->clear_flag = true;
            }
            if (canvas->allow_hard_clear) {
                canvas_clear(canvas);
                canvas->repaint_entire_canvas = true;
            }
            //ansi_debug_dump();
            break;
        }
        clear_ansi_flags(FLAG_ALL);
        break;
    case 'K':
        /* https://www.gnu.org/software/screen/manual/html_node/Control-Sequences.html
        		ESC [ Pn K                      Erase in Line
          Pn = None or 0            From Cursor to End of Line
        	 1                    From Beginning of Line to Cursor
             2                    Entire Line
        */
        if (paramidx > 1) {
            fprintf(stderr, "+++ 'K' command -- too many parameters!\n");
            ansi_debug_dump();
        }

        if (!paramidx) {
            fprintf(stderr, "+++ 'K' command -- erase from cursor to end of line\n");
            //   ansi_debug_dump();
        }

        switch (parameters[0]) {
        case 0:
            fprintf(stderr, "+++ '0K' command -- erase from cursor to end of line\n");
            r = canvas_get_raster(canvas, current_y);
            assert(r);
            /* not clear whether attributes/colors should be affected */
            for (ii = current_x; ii < r->bytes; ii++) {
                r->chardata[ii] = ' ';
//                r->bgcolors[ii] = 0;
//               r->fgcolors[ii] = 7;
                //              r->attribs[ii] = ATTRIB_NONE;
            }
            canvas->repaint_entire_canvas = true;
            break;
        case 1:
            fprintf(stderr, "+++ '1K' command -- erase from beginning of line to cursor\n");
            r = canvas_get_raster(canvas, current_y);
            assert(r);
            for (ii = 0; ii < current_x; ii++) {
                r->chardata[ii] = ' ';
                /* not clear whether attributes/colors should be affected */
                //r->bgcolors[ii] = 0;
                //r->fgcolors[ii] = 7;
                //r->attribs[ii] = ATTRIB_NONE;
            }
            canvas->repaint_entire_canvas = true;
            break;
        case 2:
            fprintf(stderr, "+++ '2K' command -- erase entire line\n");
            r = canvas_get_raster(canvas, current_y);
            assert(r);
            for (ii = 0; ii < r->bytes; ii++) {
                r->chardata[ii] = ' ';
                /* not clear whether attributes/colors should be affected */
//                r->bgcolors[ii] = 0;
//                r->fgcolors[ii] = 7;
//                r->attribs[ii] = ATTRIB_NONE;
            }
            canvas->repaint_entire_canvas = true;
            break;
        default:
            fprintf(stderr, "+++ 'K' command -- unknown parameter value '%u'\n", parameters[0]);
            ansi_debug_dump();
            break;
        }
        clear_ansi_flags(FLAG_ALL);
        break;
    default:
        printf("+++ unknown parameterized ansi command '%c'\n", c);
        ansi_debug_dump();
        assert(NULL);
        exit(1);
        break;
    }

}


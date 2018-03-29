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

int cursor_x = 0;
int cursor_y = 0;

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


uint8_t ansiflags = 0;

void dispatch_ansi_command(unsigned char c);


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


bool send_byte_to_canvas(ANSICanvas *canvas, uint16_t x, uint16_t y, unsigned char c)
{
    ANSIRaster *r = NULL;
    r = canvas_get_raster(canvas, y);

    while (!r) {
        printf("send_byte_to_canvas(%u, %u): line %u does not exist, requesting new\n", x, y, y);
        if (!canvas_add_raster(canvas)) {
            printf("*** error adding raster to canvas (line %u)\n", y);
            };
        r = canvas_get_raster(canvas, y);
        }

    printf("send_byte_to_canvas(%u, %u): retrieved raster line %u (%u bytes)\n", x, y, y, r->bytes);
    if (x > r->bytes) {
        printf("send_byte_canvas(%u, %u): raster is too short (%u bytes)\n", x, y, r->bytes);
        exit(1);
        }
    if (!raster_append_byte(r, c, 7, 0, true)) {
        printf("send_byte_to_canvas(%u, %u): error appending byte\n", x, y);
        exit(1);
        };

    printf("byte appended to canvas\n");

    return true;

}

bool ansi_to_canvas(ANSICanvas *canvas, unsigned char *buf, size_t nbytes)
{
    unsigned char c = 0, last_c = 0;
    size_t o = 0;

    while (o < nbytes) {

        /* get next character from stream */
        last_c = c;
        c = buf[o];
        printf("[FLAGS=0x%02x] %lu/%lu = 0x%02x: ", ansiflags, o, nbytes, c);
        switch(ansiflags) {
        case FLAG_NONE:
            if (c == ANSI_1B) {
                printf("ANSI_1B\n");
                set_ansi_flags(FLAG_1B);
            } else {
                if (c == '\r' || c == '\n') {
                    printf("LINE BREAK! (%u)\n", c);
                    current_x = 0;
                    current_y++;
                
                    if (!canvas_add_raster(canvas)) {
                        printf("*** error adding raster to canvas (line %u)\n", current_y);
                        exit(1);
                    };
                    
                } else {
                    printf("output character '%c'\n", c);
                    if (!send_byte_to_canvas(canvas, current_x, current_y, c)) {
                        printf("ERROR writing byte to canvas at (%u, %u)\n", current_x, current_y);
                        exit(1);
                        }                    
                    }
            }
            break;
        case FLAG_1B:
            if (c != ANSI_5B) {
                printf("error: ANSI_5B expected\n");
                exit(1);
            }
            printf("ANSI_5B\n");
            set_ansi_flags(FLAG_5B);
            init_parameters();
            break;
        case (FLAG_1B | FLAG_5B):
            if (isdigit(c)) {
                paramval = c - 0x30;
                printf("start integer parameter [%u], current_value = %u\n", paramidx, paramval);
                set_ansi_flags(FLAG_INT);
            } else {
                printf("error: expecting digit, got '%c' (0x%02x)\n", c, c);
                exit(1);
            }
            break;
        case (FLAG_1B | FLAG_5B | FLAG_INT):
            if (isalpha(c)) {
                parameters[paramidx] = paramval;
                paramcount ++;
                printf("assign integer parameter [%u] = %u\n", paramidx, parameters[paramidx]);
                paramidx++;
                printf("got alphabetical '%c', dispatching\n", c);
                dispatch_ansi_command(c);
                clear_ansi_flags(FLAG_1B | FLAG_5B | FLAG_INT);
                break;
                }
            if (c == ANSI_INTSEP) {
                /* integer seperator */
                parameters[paramidx] = paramval;
                paramcount ++;
                printf("assign integer parameter [%u] = %u\n", paramidx, parameters[paramidx]);
                paramidx++;
                paramval = 0;
                clear_ansi_flags(FLAG_INT);
            } else {
                if (isdigit(c)) {
                    paramval = (paramval * 10) + (c - 0x30);
                    printf(" cont integer parameter [%u], current_value = %u\n", paramidx, paramval);
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
    printf("BLOCK DONE\n");
    return 0; 
}

void dispatch_ansi_text_attributes()
{
    printf("--- dispatching %u text attribute parameters\n", paramcount);
    for (int i = 0; i < paramcount; i++) {
        printf("  + parameter[%u/%u] = %u\n", i, paramcount - 1, parameters[i]);
        if (parameters[i] >= 30 && parameters[i] <= 37) {
                fgcolor = parameters[i] - 30;
                printf("  * fgcolor = %u\n", fgcolor);
                goto next_parameter;
                }
        if (parameters[i] >= 40 && parameters[i] <= 47) {
                bgcolor = parameters[i] - 40;
                printf("  * bgcolor = %u\n", bgcolor);
                goto next_parameter;
                }

        switch(parameters[i]) {
        case 0:
            attributes = ATTRIB_NONE;
            fgcolor = 7;
            bgcolor = 0;
            goto next_parameter;
            break;
        case 1:
            attributes |= ATTRIB_BOLD;
            goto next_parameter;
            break;
        case 21:
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

next_parameter: ;
        }

}

void dispatch_ansi_command(unsigned char c)
{

    printf("dispatch_ansi_command('%c')\n", c);

    switch (c) {
            case 'm':
            /* text attributes */
            dispatch_ansi_text_attributes();
            break;
            default:
                printf("+++ unknown ansi command '%c'\n", c);
                exit(1);
            break;
            }

}

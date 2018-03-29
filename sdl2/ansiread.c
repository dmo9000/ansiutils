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



extern int errno;


#define CONSOLE_WIDTH		80
#define CONSOLE_HEIGHT	25

#define CHUNK_SIZE    4096
#define BUF_SIZE 		8192
#define MAX_ANSI		64				/* maximum length allowed for an ANSI sequence */
#define MAX_PARAMS	16

static char filebuf[4096];
static char ansibuf[MAX_ANSI];
int parameters[MAX_PARAMS];

#define SEQ_ERR							0
#define SEQ_NONE						1
#define SEQ_ESC_1B					2
#define SEQ_ANSI_5B					3
#define SEQ_ANSI_IPARAM			4
#define SEQ_ANSI_CMD_M			5
#define SEQ_ANSI_CMD_J			6
#define SEQ_ANSI_CMD_A			7
#define SEQ_ANSI_CMD_B			8
#define SEQ_ANSI_CMD_C			9
#define SEQ_ANSI_CMD_D			10
#define SEQ_ANSI_CMD_H			11
#define SEQ_ANSI_EXECUTED		12
#define SEQ_NOOP					 	13

static char *states[] = {
    "SEQ_ERR",
    "SEQ_NONE",
    "SEQ_ESC_1B",
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

char ansi_mode = 0;
char last_ansi_mode = 0;
int ansioffset = 0;
int paramidx = 0;
off_t offset = 0;
off_t current_escape_address = 0;

int decode_1B(char);
int decode_5B(char);
int decode_command(char);
int decode_integer_parameter(char);

int ansi_decode_cmd_m();									/* text attributes, foreground and background color handling */
int ansi_decode_cmd_J();									/* "home" command (2J) */
int ansi_decode_cmd_A();									/* move down n lines */
int ansi_decode_cmd_B();									/* move down n lines */
int ansi_decode_cmd_D();									/* move left n columns */
int ansi_decode_cmd_C();									/* move right n columns */
int ansi_decode_cmd_H();									/* set cursor position */

void init_parameters();
const char *ansi_state(int s);

int main(int argc, char *argv[])
{
    FILE *ansfile = NULL;
    char *bufptr = NULL;
    char *scanptr = NULL;
    char *endptr = NULL;
    size_t elements_read = 0;
    size_t bytes_read = 0;
    char c = 0;
    char last_c = 0;
    int i = 0;
    struct stat sbuf;
    size_t total_length = 0;

    if (argc < 2) {
        printf("usage: ansiread <filename.ans>\n");
        exit(1);
    }

    lstat(argv[1], &sbuf);
    printf("filesize = %lu\n", sbuf.st_size);
    assert(sbuf.st_size);

		total_length = sbuf.st_size;

    ansfile = fopen(argv[1], "rb");

    if (!ansfile) {
        printf("cannot open: %s: %s\n", argv[1], strerror(errno));
        exit(1);
    }


    while (!feof(ansfile) && !ferror(ansfile) && offset < total_length) {
    		printf("offset: %lu, total_length: %lu\n", offset, total_length);
        bufptr = (char *) &filebuf;
        elements_read = fread((char *) bufptr, CHUNK_SIZE, 1, ansfile);
        bytes_read = (elements_read * CHUNK_SIZE);
        printf("[%ld] bytes read to buffer\n", bytes_read);
        endptr = (char *) bufptr + (bytes_read);
        printf("\n");
        printf("[reached end of buffer, offset = %lu, bytes_read=%lu]\n", offset, bytes_read);
    }
    fclose(ansfile);
    assert(!endptr);
    exit (0);
}


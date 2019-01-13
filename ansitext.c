#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct ansicommand {
    char *command;
    char *sequence;
} ANSICommand;


/* There is a GCC warning triggered by this, "warning: missing braces around initializer"
   but it apperas to be a bug */

ANSICommand commands[] = {
    { "NONE", "\x1b[0m" },
    { "BOLD", "\x1b[1m" },
    { "NOBOLD", "\x1b[21m" },
    { "UNDERLINE", "\x1b[4m" },
    { "NOUNDERLINE", "\x1b[24m" },
    { "FG_BLACK", "\x1b[30m" },
    { "FG_RED", "\x1b[31m" },
    { "FG_GREEN", "\x1b[32m" },
    { "FG_YELLOW", "\x1b[33m" },
    { "FG_BLUE", "\x1b[34m" },
    { "FG_PURPLE", "\x1b[35m" },
    { "FG_CYAN", "\x1b[36m" },
    { "FG_WHITE", "\x1b[37m" },
    { "FG_NONE", "\x1b[37m" },
    { "TAB", "\t" },
    { "NEWLINE", "\n" },
    { NULL, NULL },
    NULL
} ;


char *ansi_command(char *msg)
{
    ANSICommand *cmdptr = NULL;
    int i = 0;

    cmdptr = commands;
    while (cmdptr != NULL && cmdptr->command != NULL) {
//			printf("  %d) search -> [%s]\n", i, cmdptr->command);
        if (strncmp(cmdptr->command, msg, strlen(msg)) == 0) {
            if (strlen(msg) == strlen(cmdptr->command)) {
                return cmdptr->sequence;
            }
        }

        cmdptr++;
        i++;
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    int i = 0;
    char *sequence = NULL;

    for (i = 1; i < argc; i++) {
//			printf("[%d] -> [%s]\n", i, argv[i]);
        sequence = ansi_command(argv[i]);
        if (sequence) {
            printf("%s", sequence);
        } else {
            printf("%s", argv[i]);
        }
    }

    exit(0);
}

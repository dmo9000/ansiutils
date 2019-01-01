#include <stdio.h>

typedef struct ansicommand {
										char *command;
										char *sequence;
									 } ANSICommand;


ANSICommand commands[] = {
														{ "NONE", "\x1b[0m" },
														{ "FG_RED", "\x1b[31m" },
														{ "FG_GREEN", "\x1b[32m" },
														{ "FG_NONE", "\x1b[37m" },
														{ "NEWLINE", "\n" },
														{ NULL, NULL }, 
														NULL
													};


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

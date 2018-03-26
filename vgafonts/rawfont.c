#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#define MAX_CHARS 256

extern int gfx_main(uint16_t, uint16_t);

int main(int argc, char *argv[])
{
	size_t font_len_expect;
	size_t font_len_actual;
	uint8_t *fontdata = NULL;
	FILE *rawfont = NULL;
	char *filename = (char *) argv[1];
	uint8_t height = 8;
	uint8_t width = 8;

	/* hardcoded for 8x8 font as first argument for now */

	font_len_expect = (width * height * MAX_CHARS);
	rawfont = fopen(filename, "rb");
	if (!rawfont) {
		perror("fopen");
		exit(1);
		}

	if (fseek(rawfont, 0, SEEK_END)) {
			perror("fseek");
			exit(1);
			}

	font_len_actual = ftell(rawfont);

	assert ((bool) (font_len_actual == font_len_expect));

	fontdata = malloc(font_len_expect);

	if (!fontdata) {
			perror("malloc");
			exit(1);
			}
				
	printf("expected and found %lu bytes\n", (font_len_expect));
	fclose(rawfont);

  gfx_main(height, width);

	exit(0);

}

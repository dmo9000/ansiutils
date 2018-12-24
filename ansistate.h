#include <stdbool.h>
#include "ansicanvas.h"

bool ansi_to_canvas(ANSICanvas *canvas, unsigned char *buf, size_t nbytes, size_t offset);
int ansi_setdebug(bool debugstate);

#include <stdint.h>
#include "utf8.h"

uint16_t convert_cp437_to_utf8(unsigned char x)
{
	return cp437_to_utf8[x];
	
}

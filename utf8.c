#include <stdint.h>
#include "utf8.h"
#include "utf8_mapping.h"

uint16_t convert_cp437_to_utf8(unsigned char x)
{
	return cp437_to_utf8[x];
}

uint8_t hi_cp437_to_utf8(unsigned char x)
{
	uint8_t uc = (cp437_to_utf8[x] >> 8);

	if (uc == 0x25) {
				return 0x96;
			 } 

	return (uint8_t) (cp437_to_utf8[x] >> 8);
}	

uint8_t lo_cp437_to_utf8(unsigned char x)
{
	return (uint8_t) (cp437_to_utf8[x] & 0x00FF);
}	

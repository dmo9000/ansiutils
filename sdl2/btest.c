#include <stdio.h>

// LITTLE ENDIAN
// 0x0f removes the top 4 bits
// 0xf0 remove the lower 4 bits
// >> shifts towards lower
// << shifts towards upper
// BIG ENDIAN  
//
// THEREFORE
// foreground color 0-15 (8-15 being the BOLD version) are stored in the 4 lowest bits
// background color 0-7 are stored in the upper 4 bits
//
// eg.
//            fg = ( color & 0x0F ) % 0x0F;
//            bg = ((color & 0xF0) >> 4) % 0x08;

int main()
{

    printf("%u\n", 0xff & 0x0f);
    printf("%u\n", (0xff & 0xf0) >> 4);


}


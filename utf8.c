#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "utf8.h"
#include "utf8_strings.h"

char *utf8_string(unsigned char x)
{
    return (char *) utf8_strings[x-128];

}

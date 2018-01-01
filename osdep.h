#define IS_BIG_ENDIAN (*(uint16_t *)"\0\xff" < 0x100)

#ifndef __ANSICOLOR_T__

#define __ANSICOLOR_T__

#define ATTRIB_NONE             0
#define ATTRIB_REVERSE          1
#define ATTRIB_BLINKING         2
#define ATTRIB_HALFINTENSITY    4
#define ATTRIB_UNDERLINE        8
#define ATTRIB_BOLD             16


typedef uint8_t color_t;
typedef color_t ibmcolor_t;
typedef color_t ansicolor_t;
typedef uint8_t attributes_t;

#endif /* __ANSICOLOR_T__ */



CFLAGS=-Wall -Werror -g -ggdb --std=c99 -I..
LDFLAGS=-lSDL2
OBJS=rawfont.o gfx_sdl.o ansiload.o

all: rawfont 

bmf/8x8.bmf: Makefile pf/8x8.pf
	( echo -ne "BMF\x00\x08\x08\x00\x01" && cat pf/8x8.pf ) > bmf/8x8.bmf

rawfont: $(OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS) -o rawfont $(OBJS) -L.. -lansicanvas

clean:
	rm -f $(OBJS) rawfont 

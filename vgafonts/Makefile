CFLAGS=-Wall -Werror -g -ggdb --std=c99
LDFLAGS=-lSDL2
OBJS=rawfont.o gfx_sdl.o

all: bmf/8x8.bmf rawfont 

bmf/8x8.bmf:
	( echo -ne "BMF\x00\x08\x08\x0100" && cat pf/8x8.pf ) > bmf/8x8.bmf

rawfont: $(OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS) -o rawfont $(OBJS)


clean:
	rm -f $(OBJS) rawfont bmf/*.bmf

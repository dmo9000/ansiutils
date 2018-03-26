CFLAGS=-Wall -Werror -g -ggdb --std=c99
LDFLAGS=-lSDL2
OBJS=rawfont.o gfx_sdl.o

rawfont: $(OBJS) 
	$(CC) $(CFLAGS) $(LDFLAGS) -o rawfont $(OBJS)

clean:
	rm -f $(OBJS) rawfont

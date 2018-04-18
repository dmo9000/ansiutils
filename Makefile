CC=gcc
#CC=clang
#CFLAGS = -g -ggdb -Wall -Werror
CFLAGS = -Wall --std=c99 -g -ggdb

TDFTOOL_OBJS=tdftool.o tdffont.o sauce.o
OBJS=gfx_sdl.o ansiload.o
ANSIREAD_OBJS=ansiread.o gfx_png.o bmf.o
LIBANSICANVAS_OBJS=ansiraster.o ansicanvas.o ansistate.o utf8.o 
LIBANSISDLCANVAS_OBJS=gfx_sdl.o rawfont.o
TESTSAMPLE=../THEDRAWFONTS/BLACKX.TDF

all: libansicanvas.a libansisdlcanvas.a tdftool rawfont ansiread bmf/8x8.bmf

sample:
	@./tdftool -c ${TESTSAMPLE} ABC

sampledebug:
	@./tdftool -d -c ${TESTSAMPLE} ABC | more

libansisdlcanvas.a: $(LIBANSISDLCANVAS_OBJS)
	ar cru libansisdlcanvas.a $(LIBANSISDLCANVAS_OBJS)

libansicanvas.a: $(LIBANSICANVAS_OBJS)
	ar cru libansicanvas.a $(LIBANSICANVAS_OBJS)

tdftool: $(TDFTOOL_OBJS) libansicanvas.a
	$(CC) -o tdftool $(TDFTOOL_OBJS) -L. -lansicanvas -lm

bmf/8x8.bmf: Makefile pf/8x8.pf
	( echo -ne "BMF\x00\x08\x08\x00\x01" && cat pf/8x8.pf ) > bmf/8x8.bmf

ansiread: $(ANSIREAD_OBJS) libansicanvas.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o ansiread $(ANSIREAD_OBJS) -L. -lansicanvas -lpng -lSDL2 -lansisdlcanvas -lm

rawfont: $(OBJS) libansicanvas.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o rawfont $(OBJS) -L. -lansicanvas -lSDL2 -lansisdlcanvas -lm

clean:
	rm -f tdftool rawfont ansiread *.o *.a *.core

veryclean: clean
	rm -rf tests/pass/*.ans
	rm -rf tests/fail/*.ans
	rm -rf tests/pass/utf8/*.ans


install:
	sudo cp tdftool /usr/bin/tdftool
	sudo cp *.a /usr/lib/

test: tdftool
	./test.sh

CFLAGS=-Wall -Werror -g -ggdb --std=c99 -I..
LDFLAGS=-lSDL2 
OBJS=rawfont.o gfx_sdl.o ansiload.o
ANSIREAD_OBJS=ansiread.o ansistate.o gfx_sdl.o bmf.o
TESTFILE=../tests/pass/cp437/kevin2.tdf.ans
#TESTFILE=../tests/pass/cp437/blackx.tdf.ans
#TESTFILE=ansifiles/ftron.ans
#TESTFILE=../tests/pass/cp437/voidx.tdf.ans


all: rawfont ansiread 

test:
	./ansiread ${TESTFILE}

debug:
	xxd ${TESTFILE}

display:
	cat ${TESTFILE}

bmf/8x8.bmf: Makefile pf/8x8.pf
	( echo -ne "BMF\x00\x08\x08\x00\x01" && cat pf/8x8.pf ) > bmf/8x8.bmf

ansiread: $(ANSIREAD_OBJS) ../libansicanvas.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o ansiread $(ANSIREAD_OBJS) -L.. -lansicanvas -lm

rawfont: $(OBJS) ../libansicanvas.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o rawfont $(OBJS) -L.. -lansicanvas -lm

clean:
	rm -f $(OBJS) rawfont ansiread 

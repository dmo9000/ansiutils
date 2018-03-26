CC=gcc
#CC=clang
#CFLAGS = -g -ggdb -Wall -Werror
CFLAGS = -Wall -Werror --std=c99 -g -ggdb

TDFTOOL_OBJS=tdftool.o tdffont.o sauce.o
LIBANSICANVAS_OBJS=ansiraster.o ansicanvas.o utf8.o

# Enable static linking. Not generally recommended, but useful for getting started with Docker containers. 
# For most cases this should be commented out or left empty. 
#LDFLAGS = -static

all: libansicanvas.a tdftool

libansicanvas.a: $(LIBANSICANVAS_OBJS) 
	ar cru libansicanvas.a $(LIBANSICANVAS_OBJS)

tdftool: $(TDFTOOL_OBJS) libansicanvas.a
	$(CC) -o tdftool $(TDFTOOL_OBJS) -L. -lansicanvas 

clean:
	rm -f tdftool *.o *.a *.core

veryclean: clean
	rm -rf tests/pass/*.ans
	rm -rf tests/fail/*.ans
	rm -rf tests/pass/utf8/*.ans


install:
	sudo cp tdftool /usr/bin/tdftool

test: tdftool
	./test.sh

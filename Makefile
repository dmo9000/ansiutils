CC=gcc
#CC=clang
#CFLAGS = -g -ggdb -Wall -Werror
CFLAGS = -Wall -Werror


# Enable static linking. Not generally recommended, but useful for getting started with Docker containers. 
# For most cases this should be commented out or left empty. 
#LDFLAGS = -static

all: tdftool

tdftool: tdftool.o tdffont.o tdfraster.o tdfcanvas.o sauce.o

clean:
	rm -f tdftool *.o *.core

veryclean: clean
	rm -rf tests/pass/*.ans
	rm -rf tests/fail/*.ans
	rm -rf tests/pass/utf8/*.ans


install:
	sudo cp tdftool /usr/bin/tdftool

test: tdftool
	./test.sh
	./mkutf8.sh

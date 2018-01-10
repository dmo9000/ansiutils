CC=gcc
CFLAGS = -g -ggdb

all: tdftool

clean:
	rm -f tdftool
	rm -f *.core

veryclean: clean
	rm -rf tests/pass/*.ans
	rm -rf tests/fail/*.ans
	rm -rf tests/pass/utf8/*.ans


install:
	sudo cp tdftool /usr/bin/tdftool

test:
	./test.sh

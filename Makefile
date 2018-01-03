CC=gcc
CFLAGS = -g -ggdb

all: tdftool

clean:
	rm -f tdftool
	rm -rf tests/pass/*
	rm -rf tests/fail/*


install:
	sudo cp tdftool /usr/bin/tdftool

test:
	./tdftool CYBSMALL.TDF "HI" 

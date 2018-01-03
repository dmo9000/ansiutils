CC=gcc
CFLAGS = -g -ggdb

all: tdftool

clean:
	rm -f tdftool
	rm -rf tests/pass/*
	rm -rf tests/fail/*



test:
	./tdftool CYBSMALL.TDF "HI" 

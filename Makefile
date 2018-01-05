CC=gcc
CFLAGS = -g -ggdb

all: tdftool

clean:
	rm -f tdftool

veryclean: clean
	rm -rf tests/pass/*
	rm -rf tests/fail/*


install:
	sudo cp tdftool /usr/bin/tdftool

test:
	./test.sh

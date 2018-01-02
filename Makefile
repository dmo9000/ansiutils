
CFLAGS = -g -ggdb

all: tdftool

clean:
	rm -f tdftool

test:
	./tdftool CYBSMALL.TDF "HI" 

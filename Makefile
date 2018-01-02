
CFLAGS = -g -ggdb

all: tdftool

clean:
	rm -f tdftool

test:
	./tdftool data/DARKSUNX.TDF "HI" 

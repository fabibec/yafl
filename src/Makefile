CC=gcc
CFLAGS=-O3 -Wall -ggdb
LDFLAGS=-Ibuild

all: build yaflc

build:
	mkdir -p build

build/parser.c: parser.y
	bison --defines=build/parser.h -o $@ $^

build/parser.o: build/parser.c
	$(CC) $(CFLAGS) -c -o $@ $^

build/lexer.c: lexer.l | build/parser.c
	flex -o $@ $<

build/lexer.o: build/lexer.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

yaflc: build/lexer.o build/parser.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -rf build

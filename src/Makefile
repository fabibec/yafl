CC=gcc
CFLAGS=-O3 -Wall -ggdb
LDFLAGS=-Ibuild -Ilib -I../vm3/vm

all: build yaflc

build:
	mkdir -p build

build/stringbuf.o: lib/stringbuf.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/ast.o: lib/ast.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/optim.o: lib/optim.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/codegen.o: lib/codegen.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/main.o: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/parser.c: parser.y
	bison --defines=build/parser.h -o $@ $^

build/parser.o: build/parser.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/lexer.c: lexer.l | build/parser.c
	flex -o $@ $<

build/lexer.o: build/lexer.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

yaflc: build/lexer.o build/parser.o build/stringbuf.o build/ast.o build/optim.o build/codegen.o build/main.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -rf build

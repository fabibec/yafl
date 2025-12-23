CC=gcc
CFLAGS=-O3 -Wall -ggdb
LDFLAGS=-Ibuild -Ilib -I../vm3/vm

all: build vm3 yafl yaflc

build:
	mkdir -p build

vm3:
	make -C ../vm3/vm

build/stringbuf.o: lib/stringbuf.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/ast.o: lib/ast.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/optim.o: lib/optim.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/utils.o: lib/utils.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/symtab.o: lib/symtab.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/codegen.o: lib/codegen.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/main.o: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/parser.c: parser.y | build
	bison --defines=build/parser.h -o $@ $^

build/parser.o: build/parser.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/lexer.c: lexer.l | build/parser.c build
	flex -o $@ $<

build/lexer.o: build/lexer.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

yafl: lib/yafl.c | build vm3
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< ../vm3/vm/libvm3.a

yaflc: build/lexer.o build/parser.o build/stringbuf.o build/ast.o build/optim.o build/codegen.o build/main.o build/utils.o build/symtab.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ ../vm3/vm/libvm3.a

clean:
	rm -rf build yafl yaflc

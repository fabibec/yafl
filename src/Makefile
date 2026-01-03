CC=gcc
CFLAGS=-O3 -Wall -ggdb
LDFLAGS=-lm -Ibuild -Ilib -I../vm3/vm

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

build/types.o: lib/types.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/hashmap.o: lib/hashmap.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/symtab.o: lib/symtab.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/codegen.o: lib/codegen.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/yaflc.o: yaflc.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/parser.c: parser.y | build
	bison --defines=build/parser.h -o $@ $^

build/parser.o: build/parser.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

build/lexer.c: lexer.l | build/parser.c build
	flex -o $@ $<

build/lexer.o: build/lexer.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $^

yafl: yafl.c ../vm3/vm/libvm3.a | build
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< ../vm3/vm/libvm3.a

yaflc: build/lexer.o build/parser.o build/stringbuf.o build/ast.o build/optim.o build/codegen.o build/yaflc.o build/utils.o build/symtab.o build/hashmap.o build/types.o ../vm3/vm/libvm3.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lm

clean:
	rm -rf build yafl yaflc *.dot *.svg
	make clean -C ../vm3/vm

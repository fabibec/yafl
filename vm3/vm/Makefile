CC=gcc
CFLAGS=-O2 -ggdb -Wall -I. -I../../src/utils -Werror --std=gnu11

OBJ=prog.o custom.o stack.o vstack.o val/str.o val/num.o val/arr.o val/real.o val/val.o val/map.o vars.o utils.o
LIBS=-lm

all: vm main

vm: vm.o libvm3.a
	$(CC) -o $@ $^ $(LIBS)

main: main.o libvm3.a
	$(CC) -o $@ $^ $(LIBS)

libvm3.a: $(OBJ) op_enum.h op_switch.h op_str.h
	ar -rc libvm3.a $(OBJ)

prog.o: includes

utils.o: ../../src/utils/utils.c
	$(CC) $(CFLAGS) -I../../src/utils -c -o $@ $<

includes:
	./mkincludes.sh

clean:
	rm -f $(OBJ) vm main vm.o libvm3.a main.o

.PHONY: includes

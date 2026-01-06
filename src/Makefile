CC = gcc
# Include all source directories
CPPFLAGS = -Ibuild -Iutils -Iast -Ianalysis -Icodegen -Ioptim -Iparser -I../vm3/vm -MMD -MP
CFLAGS = -O3 -Wall -ggdb --std=gnu23
LDLIBS = -lm
BUILD = build

COMMON_OBJS = \
	$(BUILD)/lexer.o \
	$(BUILD)/parser.o \
	$(BUILD)/stringbuf.o \
	$(BUILD)/ast.o \
	$(BUILD)/optim.o \
	$(BUILD)/codegen.o \
	$(BUILD)/builtins.o \
	$(BUILD)/utils.o \
	$(BUILD)/symtab.o \
	$(BUILD)/hashmap.o \
	$(BUILD)/types.o \
	$(BUILD)/type_checking.o \
	$(BUILD)/loop_stack.o \
	$(BUILD)/vector.o \
	$(BUILD)/logger.o

YAFL_OBJS = $(BUILD)/yafl.o
YAFLC_OBJS = $(BUILD)/yaflc.o

# default
.PHONY: all
all: $(BUILD) vm3 yafl yaflc

# build dir
$(BUILD):
	mkdir -p $@

# vm3
../vm3/vm/libvm3.a:
	$(MAKE) -C ../vm3/vm vm libvm3.a

.PHONY: vm3
vm3: ../vm3/vm/libvm3.a

# Rules for source directories
$(BUILD)/%.o: utils/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: ast/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: analysis/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: codegen/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD)/%.o: optim/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Bison
$(BUILD)/parser.c $(BUILD)/parser.h: parser/parser.y | $(BUILD)
	bison --defines=$(BUILD)/parser.h -o $(BUILD)/parser.c $<

$(BUILD)/parser.o: $(BUILD)/parser.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Flex
$(BUILD)/lexer.c: parser/lexer.l $(BUILD)/parser.h | $(BUILD)
	flex -o $@ $<

$(BUILD)/lexer.o: $(BUILD)/lexer.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

# Yafl toolchain
$(BUILD)/yaflc.o: yaflc.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(BUILD)/yafl.o: yafl.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

yafl: $(YAFL_OBJS) $(COMMON_OBJS) ../vm3/vm/libvm3.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

yaflc: $(YAFLC_OBJS) $(COMMON_OBJS) ../vm3/vm/libvm3.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

# Rebuild on header changes
-include $(BUILD)/*.d

# Clean
.PHONY: clean
clean:
	rm -rf $(BUILD) yafl yaflc *.dot *.svg
	$(MAKE) -C ../vm3/vm clean
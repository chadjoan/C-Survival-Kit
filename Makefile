
CC=gcc
CFLAGS=-Wall -ggdb

LIBFILE=lib/survival_kit.o

TOOL_EXES= \
	bin/unittests

OBJECT_FILES= \
	$(obj_exists) \
	obj/slist_builtins.o \
	obj/str.o \
	obj/generated_exception_defs.o \
	obj/features.o \
	obj/sockn.o

all: $(LIBFILE) $(TOOL_EXES)

tools: $(TOOL_EXES)

bin/%: tools_src/%.c $(LIBFILE)
	$(CC) $(CFLAGS) -I. $(LIBFILE) $< -o $@

library: $(LIBFILE)

$(LIBFILE): $(OBJECT_FILES) $(lib_exists)
	ld -r $(OBJECT_FILES) -o $(LIBFILE)
#	ar rcs $(LIBFILE) $(OBJECT_FILES)

obj/%.o: survival_kit/%.c
	$(CC) $(CFLAGS) -I. -c $< -o $@

lib_exists: | lib
	mkdir -p lib

obj_exists: | obj
	mkdir -p obj

clean:
	rm -rf lib/* && rm -rf obj/* && rm -rf bin/*


CC=gcc
CFLAGS=-Wall -rdynamic -ggdb
# CFLAGS rationale:
# -Wall      : prints all warnings.  We generally want to make sure there are no warnings.
# -rdynamic  : allows the C function backtrace_symbols_fd to look up function names when printing stack traces.
# -ggdb      : debug information.  -ggdb was chosen over -g because it has better info for gdb.

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


CC=gcc
CFLAGS=-Wall -rdynamic -ggdb
# CFLAGS rationale:
# -Wall      : prints all warnings.  We generally want to make sure there are no warnings.
# -rdynamic  : allows the C function backtrace_symbols_fd to look up function names when printing stack traces.
# -ggdb      : debug information.  -ggdb was chosen over -g because it has better info for gdb.

LIBFILE=lib/survival_kit.o

TOOL_EXES= \
	bin/unittests

OBJECT_DIRS= \
	obj \
	obj/setjmp

OBJECT_FILES= \
	obj/macro.o \
	obj/memory.o \
	obj/misc.o \
	obj/inheritance_table.o \
	obj/cstr.o \
	obj/stack_builtins.o \
	obj/fstack_builtins.o \
	obj/feature_emulation/setjmp/jmp_stack.o \
	obj/feature_emulation/setjmp/jmp_fstack.o \
	obj/feature_emulation/exception.o \
	obj/feature_emulation/thread_context.o \
	obj/feature_emulation/frame_info.o \
	obj/feature_emulation/stack_trace.o \
	obj/feature_emulation/unittest.o \
	obj/feature_emulation/init.o \
	obj/bag.o \
	obj/signal_handling.o \
	obj/flags.o \
	obj/math.o \
	obj/string.o \
	obj/trie.o \
	obj/regex.o \
	obj/path.o \
	obj/parsing/peg.o \
	obj/array_builtins.o \
	obj/streams/stream.o \
	obj/streams/text_stream.o \
	obj/streams/file_stream.o \
	obj/streams/pfile_stream.o \
	obj/streams/tcp_stream.o \
	obj/streams/ind_stream.o \
	obj/streams/init.o \
	obj/init.o

all: $(LIBFILE) $(TOOL_EXES)

tools: $(TOOL_EXES)

bin/%: tools_src/%.c $(LIBFILE) | bin
	$(CC) $(CFLAGS) -I. $(LIBFILE) -lpthread $< -o $@

library: $(LIBFILE)

$(LIBFILE): $(OBJECT_FILES) | $(OBJECT_DIRS) lib
	ld -r $(OBJECT_FILES) -o $(LIBFILE)
#	ar rcs $(LIBFILE) $(OBJECT_FILES)

obj/%.o: survival_kit/%.c | $(OBJECT_DIRS)
	$(CC) $(CFLAGS) -I. -c $< -o $@

bin:
	mkdir -p bin

lib:
	mkdir -p lib

$(OBJECT_DIRS):
	mkdir -p obj/feature_emulation/setjmp && mkdir -p obj/feature_emulation && mkdir -p obj/streams && mkdir -p obj/parsing

clean:
	rm -rf lib && rm -rf obj && rm -rf bin


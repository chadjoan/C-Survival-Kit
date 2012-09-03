
CC=gcc
CFLAGS=

LIBFILE=lib/libsurvival_kit.a
OBJECT_FILES= \
	$(obj_exists) \
	obj/str.o \
	obj/generated_exception_defs.o \
	obj/features.o \
	obj/sockn.o

all: $(LIBFILE)

library: $(LIBFILE)

lib/libsurvival_kit.a: $(OBJECT_FILES) $(lib_exists)
	ar rcs lib/libsurvival_kit.a $<	

obj/%.o: survival_kit/%.c
	$(CC) $(CFLAGS) -I. -c $< -o $@

lib_exists: | lib
	mkdir -p lib

obj_exists: | obj
	mkdir -p obj

clean:
	rm -rf lib/* && rm -rf obj/*

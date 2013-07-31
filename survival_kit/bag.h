/**
This module defines a "bag", which is a way of storing any number of
weakly-typed fields (declared at start-time) into a single aggregate.

Bags are useful in situations where multiple modules must store state in a
unified context object, but must not know about other modules' state unless
they explicitly depend on the other module.  This eliminates unnecessary
circular dependencies.

An idiomatic way to define fields in a bag and use them would look like this:

// Some common module defines a unique bag layout:
// common.h
#include "survival_kit/bag.h"
extern skit_bag_layout *my_bag_layout;

// common.c
#include "survival_kit/bag.h"
skit_bag_layout *my_bag_layout = NULL;

// Another module, x, needs to define a field within common.h's bag layout.
// In x.h file:
#include "survival_kit/bag.h"

my_type *my_get_foo(skit_bag *bag);
void my_set_foo(skit_bag *bag, my_type *foo);

// In x.c file:
#include "survival_kit/assert.h"
#include "survival_kit/bag.h"

static ssize_t my_foo_bag_id = -1;

my_type *my_get_foo(skit_bag *bag)
{
	sASSERT(my_foo_bag_id >= 0);
	return SKIT_BAG_GET_FIELD(bag, my_foo_bag_id, my_type*);
}

void my_set_foo(skit_bag *bag, my_type *foo)
{
	sASSERT(my_foo_bag_id >= 0);
	SKIT_BAG_SET_FIELD(bag, my_foo_bag_id, my_type*, foo);
}

void my_x_module_init()
{
	// This should get called at program start.
	// It must be executed before any instances of the context are created,
	//   and therefore before my_get_foo or my_set_foo are ever called.
	//                     (   bag layout,  field identifier    type of the      name of
	//                                       from your module, field to store,  the field)
	SKIT_BAG_REGISTER_FIELD(&my_bag_layout,    &my_foo_bag_id,      my_type*,        foo );
	...
}

// In module y, we need to put declare another field in common.h's bag layout.
// However, we don't want x to be able to see our field.
// In y.h file:
#include "survival_kit/bag.h"

uint32_t my_get_bar(skit_bag *bag);
void my_set_bar(skit_bag *bag, uint32_t bar);

// In y.c file:
#include "survival_kit/assert.h"
#include "survival_kit/bag.h"

static ssize_t my_bar_bag_id = -1;

uint32_t my_get_bar(skit_bag *bag)
{
	sASSERT(my_foo_bag_id >= 0);
	return SKIT_BAG_GET_FIELD(bag, my_bar_bag_id, uint32_t);
}

void my_set_bar(skit_bag *bag, uint32_t bar)
{
	sASSERT(my_foo_bag_id >= 0);
	SKIT_BAG_SET_FIELD(bag, my_bar_bag_id, uint32_t, bar);
}

void my_y_module_init()
{
	// This should get called at program start.
	// It must be executed before any instances of the context are created,
	//   and therefore before my_get_bar or my_set_bar are ever called.
	//                     (   bag layout,  field identifier    type of the      name of
	//                                       from your module, field to store,  the field)
	SKIT_BAG_REGISTER_FIELD(&my_bag_layout,    &my_bar_bag_id,      uint32_t,        bar );
	...
}

*/
#ifndef SKIT_BAG_INCLUDED
#define SKIT_BAG_INCLUDED

#include "survival_kit/array_builtins.h"
#include "survival_kit/streams/stream.h"

#include <inttypes.h>
#include <stddef.h>

typedef struct skit_bag_layout skit_bag_layout;
struct skit_bag_layout
{
	size_t          bag_size;
	skit_u32_loaf   field_offsets;
	skit_cstr_loaf  field_types;
	skit_cstr_loaf  field_names;
	char            *bag_name;
};

typedef struct skit_bag skit_bag;
struct skit_bag
{
	skit_bag_layout *layout;
	char            contents[1];
};

skit_bag_layout *skit_bag_layout_new();

void skit_bag_layout_ctor(skit_bag_layout *layout);

/**
Used by bags implementing the
  bag_name ## _reg_field(...)
function.
*/
void skit_bag_layout_reg_field(
	skit_bag_layout **layout_ptr,
	ssize_t *field_id_ptr,
	size_t field_size,
	const char *type_str,
	const char *field_name);

/**
Prints a human-readable representation of the given bag layout in a format
resembling a C-struct with explicit member offsets.
*/
void skit_bag_layout_dump(const skit_bag_layout *layout, skit_stream *output);

skit_bag *skit_bag_new(skit_bag_layout *layout);

void skit_bag_free(skit_bag *bag);

void *skit_bag_get_field(skit_bag *ctx, ssize_t field_id);

void skit_bag_set_field(skit_bag *ctx, ssize_t field_id, char *new_value, size_t size);

#define SKIT_BAG_REGISTER_FIELD(bag_layout_ptr, field_id_ptr, field_type, field_name) \
	(skit_bag_layout_reg_field((bag_layout_ptr), (field_id_ptr), sizeof(field_type), #field_type, #field_name))

#define SKIT_BAG_GET_FIELD(bag_instance, field_id, field_type) \
	(*(field_type*)skit_bag_get_field((bag_instance), (field_id)))

#define SKIT_BAG_SET_FIELD(bag_instance, field_id, field_type, new_value) \
	do { \
		field_type skit__bag_tmp = new_value; \
		skit_bag_set_field((bag_instance), (field_id), (char*)&(skit__bag_tmp), sizeof(field_type) ); \
	} while (0)

void skit_bag_unittest();

#endif

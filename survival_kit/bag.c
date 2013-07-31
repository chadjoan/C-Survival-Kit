
#if defined(__DECC)
#pragma module skit_bag
#endif

#include "survival_kit/bag.h"

#include "survival_kit/assert.h"
#include "survival_kit/array_builtins.h"

#include <inttypes.h>
#include <stddef.h>

skit_bag_layout *skit_bag_layout_new()
{
	skit_bag_layout *result = skit_malloc(sizeof(skit_bag_layout));
	skit_bag_layout_ctor(result);
	return result;
}

void skit_bag_layout_ctor(skit_bag_layout *layout)
{
	layout->bag_size = 0;
	layout->field_offsets = skit_u32_loaf_new();
	layout->field_types = skit_cstr_loaf_new();
	layout->field_names = skit_cstr_loaf_new();
	layout->bag_name = "<unnamed bag>";
}

void skit_bag_layout_reg_field(
	skit_bag_layout **layout_ptr,
	ssize_t *field_id_ptr,
	size_t field_size,
	const char *type_str,
	const char *field_name)
{
	sASSERT(layout_ptr != NULL);
	sASSERT(field_id_ptr != NULL);
	sASSERT(type_str != NULL);
	sASSERT(field_name != NULL);
	
	if ( *layout_ptr == NULL )
		*layout_ptr = skit_bag_layout_new();
	
	skit_bag_layout *layout = *layout_ptr;
	
	ssize_t n_fields = skit_u32_loaf_len(layout->field_offsets);
	*field_id_ptr = n_fields;
	
	skit_u32_loaf_put(&layout->field_offsets, layout->bag_size);
	skit_cstr_loaf_put(&layout->field_types, (char*)type_str);
	skit_cstr_loaf_put(&layout->field_names, (char*)field_name);
	
	(layout->bag_size) += field_size;
}

void skit_bag_layout_dump(const skit_bag_layout *layout, skit_stream *output)
{
	if (layout == NULL)
	{
		skit_stream_appendln(output,sSLICE("(NULL skit_bag_layout)"));
		return;
	}

	skit_stream_appendf(output,"struct %s (%ld bytes)\n",
		layout->bag_name, layout->bag_size);
	skit_stream_appendln(output, sSLICE("{"));
	
	ssize_t n_fields = skit_u32_loaf_len(layout->field_offsets);
	uint32_t *offsets = skit_u32_loaf_ptr(layout->field_offsets);
	char **field_types = skit_cstr_loaf_ptr(layout->field_types);
	char **field_names = skit_cstr_loaf_ptr(layout->field_names);
	
	ssize_t i;
	for ( i = 0; i < n_fields; i++ )
		skit_stream_appendf(output, "0x%04X: %s %s;\n",
			offsets[i], field_types[i], field_names[i]);
	
	skit_stream_appendln(output, sSLICE("}"));
}

skit_bag *skit_bag_new(skit_bag_layout *layout)
{
	sASSERT(layout != NULL);
	skit_bag *result = skit_malloc(sizeof(skit_bag) + layout->bag_size - 1);
	result->layout = layout;
	memset(result->contents, 0, layout->bag_size);
	return result;
}

void skit_bag_free(skit_bag *bag)
{
	skit_free(bag);
}

void *skit_bag_get_field(skit_bag *ctx, ssize_t field_id)
{
	sASSERT(ctx != NULL);
	sASSERT(ctx->layout != NULL);
	sASSERT_GE(field_id, 0, "%ld");
	skit_bag_layout *layout = ctx->layout;
	uint32_t *offsets = skit_u32_loaf_ptr(layout->field_offsets);
	ssize_t n_offsets = skit_u32_loaf_len(layout->field_offsets);
	sASSERT_LT(field_id, n_offsets, "%ld");
	return &ctx->contents[offsets[field_id]];
}

void skit_bag_set_field(skit_bag *ctx, ssize_t field_id, char *new_value, size_t size)
{
	sASSERT(ctx != NULL);
	sASSERT(ctx->layout != NULL);
	sASSERT_GE(field_id, 0, "%ld");
	skit_bag_layout *layout = ctx->layout;
	uint32_t *offsets = skit_u32_loaf_ptr(layout->field_offsets);
	ssize_t n_offsets = skit_u32_loaf_len(layout->field_offsets);
	sASSERT_LT(field_id, n_offsets, "%ld");
	memcpy( &ctx->contents[offsets[field_id]], new_value, size );
}

static skit_bag_layout *skit_test_bag_layout = NULL;
static ssize_t skit_test_bag_intx = -1;
static ssize_t skit_test_bag_inty = -1;
static ssize_t skit_test_bag_cstr = -1;

#include "survival_kit/streams/pfile_stream.h"

void skit_bag_unittest()
{
	printf("skit_bag_unittest()\n");
	
	SKIT_BAG_REGISTER_FIELD(&skit_test_bag_layout, &skit_test_bag_intx, int32_t, x);
	SKIT_BAG_REGISTER_FIELD(&skit_test_bag_layout, &skit_test_bag_inty, int32_t, y);
	SKIT_BAG_REGISTER_FIELD(&skit_test_bag_layout, &skit_test_bag_cstr, char*, str);
	
	/*skit_bag_layout_dump(skit_test_bag_layout, skit_stream_stdout);*/
	
	skit_bag *bag = skit_bag_new(skit_test_bag_layout);

	SKIT_BAG_SET_FIELD(bag, skit_test_bag_intx, int32_t, -9);
	SKIT_BAG_SET_FIELD(bag, skit_test_bag_inty, int32_t, 42);
	SKIT_BAG_SET_FIELD(bag, skit_test_bag_cstr, char*, "Hi!");
	
	sASSERT_EQ(SKIT_BAG_GET_FIELD(bag, skit_test_bag_intx, int32_t), -9, "%d");
	sASSERT_EQ(SKIT_BAG_GET_FIELD(bag, skit_test_bag_inty, int32_t), 42, "%d");
	sASSERT_EQ_CSTR(SKIT_BAG_GET_FIELD(bag, skit_test_bag_cstr, char*), "Hi!");
	
	skit_bag_free(bag);
	
	printf("  skit_bag_unittest passed!\n");
	printf("\n");
}

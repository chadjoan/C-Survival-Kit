
#include <stdlib.h>

#include "survival_kit/memory.h"

void *skit_malloc(size_t size)
{
	return malloc(size);
}

void skit_free(void *mem)
{
	free(mem);
}

#include <stdio.h> /* snprintf */
#include <pthread.h>
#include "survival_kit/init.h"
#include "survival_kit/assert.h"
#include "survival_kit/misc.h"
#include "survival_kit/cstr.h"

#define SKIT_SCRATCH_BUF_SIZE 256
static void skit_cleanup_scratch_buf(void *scratch_buf) { skit_free(scratch_buf); }
static pthread_key_t skit_cstr_scratch_buf_key;

void skit_cstr_init()
{
	pthread_key_create(&skit_cstr_scratch_buf_key, &skit_cleanup_scratch_buf);
}

void skit_cstr_thread_init()
{
	pthread_setspecific(skit_cstr_scratch_buf_key, skit_malloc(SKIT_SCRATCH_BUF_SIZE));
}

static char *skit_int_to_fmt(int64_t val, const char *fmt)
{
	sASSERT(skit_thread_init_was_called());
	char *str_buf = (char*)pthread_getspecific(skit_cstr_scratch_buf_key);
	if ( snprintf(str_buf, SKIT_SCRATCH_BUF_SIZE, fmt, val) == -1 )
	{
		str_buf[0] = '\0';
		return str_buf; /* TODO: there's probably a better thing to do here than return the empty string.  Maybe throw an exception? */
	}
	return str_buf;
}

char *skit_int_to_cstr(int64_t val)
{
	return skit_int_to_fmt(val, "%li");
}

char *skit_uint_to_cstr(uint64_t val)
{
	return skit_int_to_fmt((int64_t)val, "%uli");
}

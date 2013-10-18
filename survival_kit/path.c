
#if defined(__DECC)
#pragma module skit_path
#endif

#include "survival_kit/feature_emulation.h"
#include "survival_kit/memory.h"
#include "survival_kit/string.h"

#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>

#ifdef __VMS
/* TODO: Some of this VMS stuff should move to a common place. */

/* See
http://h71000.www7.hp.com/doc/83final/5763/5763pro_contents_002.html#toc_part
for a list of functions on VMS, some of which are handy for handling paths.
*/
#include <unixlib.h>

#if defined(__INITIAL_POINTER_SIZE)
#pragma pointer_size save
#pragma pointer_size 32
#endif
typedef char *  skit_char_ptr32;
typedef void *  skit_void_ptr32;

/*
static skit_void_ptr32 skit_malloc32( int32_t size )
{
	return _malloc32(size);
}

static void skit_free32( skit_void_ptr32 mem )
{
	free(mem);
}

memcpy
*/

#if defined(__INITIAL_POINTER_SIZE)
#pragma pointer_size restore
#endif

#endif


typedef struct skit_path_receive_struct skit_path_receive_struct;
struct skit_path_receive_struct
{
	skit_loaf  buffer;
	skit_slice path;
};

static void skit_cleanup_receive_buf(void *path_receive_buf_v)
{
	skit_path_receive_struct *path_receive_buf = path_receive_buf_v;
	skit_loaf_free(&path_receive_buf->buffer);
	skit_free(path_receive_buf);
}
static pthread_key_t skit_path_receive_buf_key;

void skit_path_module_init()
{
	pthread_key_create(&skit_path_receive_buf_key, &skit_cleanup_receive_buf);
}

void skit_path_thread_init()
{
	skit_path_receive_struct *path_receive_buf = skit_malloc(sizeof(skit_path_receive_struct));
	path_receive_buf->buffer = skit_loaf_alloc(128);
	path_receive_buf->path = skit_slice_of(path_receive_buf->buffer.as_slice, 0, 0);
	pthread_setspecific(skit_path_receive_buf_key, path_receive_buf);
}


static int skit_path_receive( char *path )
{
	printf("skit_path_receive('%s')\n",path);
	skit_path_receive_struct *path_receive_buf = pthread_getspecific(skit_path_receive_buf_key);
	path_receive_buf->path = skit_loaf_assign_cstr(&path_receive_buf->buffer, path);
	return 1;
}

static skit_slice skit_get_received_path()
{
	skit_path_receive_struct *path_receive_buf = pthread_getspecific(skit_path_receive_buf_key);
	return path_receive_buf->path;
}

/*
The decc$from_vms interface is not thread safe and might just be poorly
planned.

We can sort-of fix it by using thread-local storage and some complicated
allocations-copies-etc to get it to work.

This function does all of that dirty-work and presents a clean API for
converting VMS paths to POSIX paths.
*/
static skit_slice skit_path_vms_to_posix( skit_loaf *path_buf, skit_slice path )
sSCOPE
	SKIT_USE_FEATURE_EMULATION;

#ifdef __VMS
	ssize_t path_len = sSLENGTH(path);
	if ( path_len == 0 )
		sRETURN(path);

	/* Prep the string for passing as a C-string. */
	/* VMS functions don't take length arguments, */
	/*  so we must allocate-and-copy to guarantee a trailing '\0'. */
	char *path_cstr = skit_malloc(path_len+1);
	sSCOPE_EXIT(skit_free(path_cstr));
	
	memcpy(path_cstr, sSPTR(path), path_len);
	path_cstr[path_len] = '\0';

	/* Pass the strings through decc$from_vms */
#define SKIT_DONT_EXPAND_WILDCARDS  (0)
#define SKIT_EXPAND_WILDCARD        (1)
	skit_slice result = skit_slice_null();

	int n_paths = decc$from_vms(path_cstr, skit_path_receive, SKIT_DONT_EXPAND_WILDCARDS);
	if ( n_paths > 0 )
	{
		skit_slice rpath = skit_get_received_path();
		/*printf("rpath = '%.*s'\n", sSLENGTH(rpath), sSPTR(rpath));*/
		result = skit_loaf_assign_slice(path_buf, rpath);
	}
	else
		result = path;

#undef SKIT_DONT_EXPAND_WILDCARDS
#undef SKIT_EXPAND_WILDCARDS

	sRETURN(result);
#else
	if ( 0 ) /* Shut up compiler warnings. */
	{
		(void)skit_path_receive(NULL);
		skit_slice result = skit_get_received_path();
		(void)result;
	}

	sRETURN(path);
#endif
sEND_SCOPE

/* TODO: PORTABILITY: handle Windows? */
skit_slice skit_path_join( skit_loaf *path_join_buf, skit_slice a, skit_slice b)
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	
#ifdef __VMS
	SKIT_LOAF_ON_STACK(lhs_buf, 128);
	SKIT_LOAF_ON_STACK(rhs_buf, 128);
	sSCOPE_EXIT_BEGIN
		skit_loaf_free(&lhs_buf);
		skit_loaf_free(&rhs_buf);
	sEND_SCOPE_EXIT

	skit_slice lhs = skit_path_vms_to_posix(&lhs_buf, a);
	skit_slice rhs = skit_path_vms_to_posix(&rhs_buf, b);
#else
	if ( 0 ) /* Shut up compiler warnings. */
	{
		skit_loaf buf = skit_loaf_null();
		skit_path_vms_to_posix(&buf, a);
	}
	
	skit_slice lhs = a;
	skit_slice rhs = b;
#endif
	
	char *rhs_ptr = (char*)sSPTR(rhs);
	ssize_t rhs_len = sSLENGTH(rhs);
	if ( rhs_len > 0 && rhs_ptr[0] == '/' )
		sTHROW(SKIT_EXCEPTION, "Could not join path '%.*s' to '%.*s' because the right-side has a leading slash (/).",
			sSLENGTH(lhs), sSPTR(lhs),
			sSLENGTH(rhs), sSPTR(rhs));
	
	if ( rhs_len > 1 && rhs_ptr[0] == '.' && rhs_ptr[1] == '/' )
	{
		rhs = skit_slice_of(rhs, 2, SKIT_EOT);
		rhs_ptr = (char*)sSPTR(rhs);
		rhs_len = sSLENGTH(rhs);
	}
	
	char *lhs_ptr = (char*)sSPTR(lhs);
	ssize_t lhs_len = sSLENGTH(lhs);
	int lhs_is_root = 0;
	if ( lhs_len > 0 )
	{
		if ( lhs_len > 1 )
			lhs = skit_slice_rtrimx(lhs, sSLICE("/"));
		else if ( lhs_ptr[0] == '/' )
			lhs_is_root = 1;
	}
	
	skit_slice result = skit_slice_of(path_join_buf->as_slice, 0, 0);
	skit_slice_buffered_append(path_join_buf, &result, lhs);
	if ( lhs_len > 0 && rhs_len > 0 && !lhs_is_root )
		skit_slice_buffered_append(path_join_buf, &result, sSLICE("/"));
	skit_slice_buffered_append(path_join_buf, &result, rhs);
	
	sRETURN(result);
sEND_SCOPE

void skit_path_join_test()
sSCOPE
	SKIT_USE_FEATURE_EMULATION;
	skit_loaf buf = skit_loaf_alloc(128);
	
	sASSERT_EQS(skit_path_join(&buf, sSLICE("x"), sSLICE("y")), sSLICE("x/y"));
	sASSERT_EQS(skit_path_join(&buf, sSLICE("x"), sSLICE("")),  sSLICE("x"));
	sASSERT_EQS(skit_path_join(&buf, sSLICE(""), sSLICE("y")),  sSLICE("y"));
	sASSERT_EQS(skit_path_join(&buf, sSLICE("x/"), sSLICE("y")), sSLICE("x/y"));
	sASSERT_EQS(skit_path_join(&buf, sSLICE("/"), sSLICE("y")), sSLICE("/y"));
	sASSERT_EQS(skit_path_join(&buf, sSLICE("x"), sSLICE("./y")), sSLICE("x/y"));
	
	int caught_exception = 0;
	sTRY
		skit_path_join(&buf, sSLICE("x"), sSLICE("/y"));
	sCATCH( SKIT_EXCEPTION, e )
		caught_exception = 1;
	sEND_TRY
	sASSERT(caught_exception);

#ifdef __VMS
	/* Change cwd to a place that VMS systems should have, and build a path that should exist. */
	/* decc$from_vms will fail if it doesn't receive existing paths.  *rolls eyes* (Hope you like disk I/O!) */
	char cwd_save[1024];
	sCTRACE(!getcwd(cwd_save,1024));
	sCTRACE(chdir("SYS$COMMON:[SYSMGR]"));
	sSCOPE_EXIT(chdir(cwd_save));

	sASSERT_EQS(skit_path_join(&buf, sSLICE("SYS$COMMON:[SYSMGR]"), sSLICE("SYSTARTUP_VMS.COM")), sSLICE("/sys$common/sysmgr/systartup_vms.com"));
	
	/* TODO: best-effort attempt to parse non-existing VMS paths. (Ex: assume logicals exist at root level, and so on.) */
	/* TODO: or maybe just attempt to concatenate VMS paths without converting to POSIX first?  So terrifying, but it might be necessary for a lot of cases. */
#endif

	skit_loaf_free(&buf);
	
	printf("  skit_path_join_test passed.\n");
sEND_SCOPE

void skit_path_unittest()
{
	SKIT_USE_FEATURE_EMULATION;
	printf("skit_path_unittest()\n");
	sTRACE(skit_path_join_test());
	printf("  skit_path_unittest passed!\n");
	printf("\n");
}
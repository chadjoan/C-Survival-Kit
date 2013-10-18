
#ifndef SKIT_PATH_INCLUDED
#define SKIT_PATH_INCLUDED

#include "survival_kit/string.h"

/**
Cross-platform way to join paths.

On OpenVMS, this will always return a POSIX style path.
*/
skit_slice skit_path_join( skit_loaf *path_join_buf, skit_slice a, skit_slice b);

/* Let these be called by other functions like skit_init(). */
void skit_path_module_init();
void skit_path_thread_init();
void skit_path_unittest();
#endif

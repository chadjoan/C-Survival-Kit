
#ifndef SKIT_PATH_INCLUDED
#define SKIT_PATH_INCLUDED

#include "survival_kit/string.h"

/// Cross-platform way to join paths.
///
/// On OpenVMS, this will always return a POSIX style path.
///
/// Because of the large likelyhood that the resulting path will be used
/// as an argument in C-functions expecting null-terminated char*'s, this
/// function is guaranteed to leave a 0-byte ('\0') in the buffer just
/// after the returned slice.
skit_slice skit_path_join( skit_loaf *path_join_buf, skit_slice a, skit_slice b);

/* Let these be called by other functions like skit_init(). */
void skit_path_module_init();
void skit_path_thread_init();
void skit_path_unittest();
#endif

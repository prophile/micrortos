#include "sys_auto.h"

#if defined(SYS_SELECTED_POSIX_AMD64)
#include "sys_posix_amd64_impl.h"
#elif defined(SYS_SELECTED_POSIX_I386)
#include "sys_posix_i386_impl.h"
#else
#error Could not detect platform
#endif

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#ifdef __x86_64__
#include "sys_posix_amd64.h"
#else
#error POSIX only currently supported for AMD64
#endif
#else
#error No current support for non-POSIX
#endif

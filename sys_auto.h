#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#ifdef __x86_64__
#include "sys_posix_amd64.h"
#elif defined(__x86__) || defined(__i386__)
#include "sys_posix_i386.h"
#else
#error POSIX only currently supported for x86
#endif
#else
#error No current support for non-POSIX
#endif

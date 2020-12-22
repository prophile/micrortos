#ifndef __INCLUDED_SYS_AUTO_H
#define __INCLUDED_SYS_AUTO_H

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#ifdef __x86_64__
#include "sys_posix_amd64.h"
#define SYS_SELECTED_POSIX_AMD64
#elif defined(__x86__) || defined(__i386__)
#include "sys_posix_i386.h"
#define SYS_SELECTED_POSIX_I386
#else
#error POSIX only currently supported for x86
#endif
#else
#error No current support for non-POSIX
#endif

#endif

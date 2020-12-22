#ifndef __INCLUDED_SYS_POSIX_AMD64_H
#define __INCLUDED_SYS_POSIX_AMD64_H

#include "sys_posix_common.h"

typedef volatile struct _SYS_context_x86_64 {
    uint64_t ip;
    uint64_t rbx;
    uint64_t rsp;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
} SYS_context_t;

extern void SYS_context_get(SYS_context_t*);
extern void SYS_context_set(SYS_context_t*) __attribute__((noreturn));
extern void _SYS_context_bootstrap();

void SYS_context_init(SYS_context_t* ctx, void (*callable)(void*), void* arg, void* stack, size_t stacksize);

#endif

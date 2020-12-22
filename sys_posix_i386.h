#ifndef __INCLUDED_SYS_POSIX_I386_H
#define __INCLUDED_SYS_POSIX_I386_H

#include "sys_posix_common.h"

typedef volatile struct _SYS_context_x86_64 {
    uint32_t ip;
    uint32_t esp;
    uint32_t ebp;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebx;
} SYS_context_t;

extern void
SYS_context_get(SYS_context_t*);
extern void
SYS_context_set(SYS_context_t*) __attribute__((noreturn));
extern void
_SYS_context_bootstrap();

void SYS_context_init(SYS_context_t* ctx,
    void (*callable)(void*),
    void* arg,
    void* stack,
    size_t stacksize);

#endif

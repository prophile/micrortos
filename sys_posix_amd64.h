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

static void SYS_context_init(SYS_context_t* ctx, void (*callable)(void*), void* arg, void* stack, size_t stacksize)
{
    ctx->ip = (uint64_t)&_SYS_context_bootstrap;
    ctx->rbx = (uint64_t)callable;
    ctx->rsp = (uint64_t)((uint8_t*)stack + stacksize);
    ctx->rbp = (uint64_t)((uint8_t*)stack + stacksize);
    ctx->r12 = (uint64_t)arg;
    ctx->r13 = 0;
    ctx->r14 = 0;
    ctx->r15 = 0;
}

__asm__(
    "_SYS_context_get:\n"
    "    movq (%rsp), %rax\n"
    "    movq %rax, (%rdi)\n"
    "    movq %rbx, 8(%rdi)\n"
    "    movq %rsp, %rax\n"
    "    addq $8, %rax\n"
    "    movq %rax, 16(%rdi)\n"
    "    movq %rbp, 24(%rdi)\n"
    "    movq %r12, 32(%rdi)\n"
    "    movq %r13, 40(%rdi)\n"
    "    movq %r14, 48(%rdi)\n"
    "    movq %r15, 56(%rdi)\n"
    "    ret\n"
    "\n"
    "_SYS_context_set:\n"
    "    movq (%rdi), %rax\n"
    "    movq 8(%rdi), %rbx\n"
    "    movq 16(%rdi), %rsp\n"
    "    movq 24(%rdi), %rbp\n"
    "    movq 32(%rdi), %r12\n"
    "    movq 40(%rdi), %r13\n"
    "    movq 48(%rdi), %r14\n"
    "    movq 56(%rdi), %r15\n"
    "    jmp *%rax\n"
    "\n"
    "__SYS_context_bootstrap:\n"
    "    movq %rbx, %rax\n"
    "    movq %r12, %rdi\n"
    "    callq *%rax\n"
    "    ud2\n"
);

#endif

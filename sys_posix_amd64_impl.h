#include "sys_posix_amd64.h"

void SYS_context_init(SYS_context_t* ctx,
    void (*callable)(void*, void*),
    void* arg1,
    void* arg2,
    void* stack,
    size_t stacksize)
{
    ctx->ip = (uint64_t)&_SYS_context_bootstrap;
    ctx->rbx = (uint64_t)callable;
    ctx->rsp = (uint64_t)((uint8_t*)stack + stacksize);
    ctx->rbp = (uint64_t)((uint8_t*)stack + stacksize);
    ctx->r12 = (uint64_t)arg1;
    ctx->r13 = (uint64_t)arg2;
    ctx->r14 = 0;
    ctx->r15 = 0;
}

__asm__("_SYS_context_get:\n"
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
        "    xorq %rax, %rax\n"
        "    ret\n"
        "\n"
        "_SYS_context_set:\n"
        "    movq (%rdi), %rdx\n"
        "    movq 8(%rdi), %rbx\n"
        "    movq 16(%rdi), %rsp\n"
        "    movq 24(%rdi), %rbp\n"
        "    movq 32(%rdi), %r12\n"
        "    movq 40(%rdi), %r13\n"
        "    movq 48(%rdi), %r14\n"
        "    movq 56(%rdi), %r15\n"
        "    movq %rsi, %rax\n"
        "    jmp *%rdx\n"
        "\n"
        "__SYS_context_bootstrap:\n"
        "    movq %rbx, %rax\n"
        "    movq %r12, %rdi\n"
        "    movq %r13, %rsi\n"
        "    callq *%rax\n"
        "    ud2\n");

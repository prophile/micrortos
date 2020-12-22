#include "sys_posix_i386.h"

void SYS_context_init(SYS_context_t* ctx, void (*callable)(void*), void* arg, void* stack, size_t stacksize)
{
    ctx->ip = (uint32_t)&_SYS_context_bootstrap;
    ctx->ebx = (uint32_t)callable;
    ctx->esp = (uint32_t)((uint8_t*)stack + stacksize);
    ctx->ebp = (uint32_t)((uint8_t*)stack + stacksize);
    ctx->edi = (uint32_t)arg;
    ctx->esi = 0;
}

__asm__(
    "_SYS_context_get:\n"
    "    movl  4(%esp), %eax\n"
    "    movl  (%esp), %edx\n"
    "    movl  %edx, (%eax)\n"
    "    movl  %esp, %edx\n"
    "    addl  $4, %edx\n"
    "    movl  %edx, 4(%eax)\n"
    "    movl  %ebp, 8(%eax)\n"
    "    movl  %edi, 12(%eax)\n"
    "    movl  %esi, 16(%eax)\n"
    "    movl  %ebx, 20(%eax)\n"
    "    ret\n"
    "\n"
    "_SYS_context_set:\n"
    "    movl  4(%esp), %eax\n"
    "    movl  (%eax), %edi\n"
    "    movl  4(%eax), %esp\n"
    "    movl  8(%eax), %ebp\n"
    "    movl  12(%eax), %edi\n"
    "    movl  16(%eax), %esi\n"
    "    movl  20(%eax), %ebx\n"
    "    jmp   *%eax\n"
    "\n"
    "__SYS_context_bootstrap:\n"
    "    pushl  %edi\n"
    "    calll  *%ebx\n"
    "    int3\n"
);

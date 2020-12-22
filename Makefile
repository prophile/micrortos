CC=clang
CFLAGS=-gfull -Os -fno-unwind-tables -fno-stack-protector -fomit-frame-pointer -fno-rtti -fno-exceptions -Wall -Werror -flto
LDFLAGS=-gfull -Os -Wall -Werror -flto
SOURCES=main.c lock.c rtos.c rtos_yield.c rtos_sched.c syscalls_data.c syscalls_exit.c syscalls_futex.c syscalls_sleep.c sys_auto.c

all: rtos

Dependencies.mk: $(SOURCES)
	"$(CC)" -MM $^ > $@

include Dependencies.mk

rtos: $(SOURCES:.c=.o)
	"$(CC)" $(LDFLAGS) -o $@ $^

%.o: %.c
	"$(CC)" $(CFLAGS) -c -o $@ $<

clean:
	rm -f rtos *.o Dependencies.mk

.PHONY: clean rtos

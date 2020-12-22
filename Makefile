CC=clang
CFLAGS=-gfull -Os -fno-unwind-tables -fno-stack-protector -fomit-frame-pointer -fno-rtti -fno-exceptions -Wall -Werror -flto
LDFLAGS=-gfull -Os -Wall -Werror -flto
SOURCES_LIB=\
	rtos_yield.c \
	rtos_sched.c \
	syscalls_data.c \
	syscalls_exit.c \
	syscalls_futex.c \
	syscalls_sleep.c \
	sys_auto.c \
	rtos.c
SOURCES_EXAMPLE=main.c lock.c
DYLIB=librtos.dylib

all: rtos

Dependencies.mk: $(SOURCES_LIB) $(SOURCES_EXAMPLE)
	"$(CC)" -MM $^ > $@

include Dependencies.mk

rtos: $(SOURCES_EXAMPLE:.c=.o) $(DYLIB)
	"$(CC)" $(LDFLAGS) -o $@ $^

$(DYLIB): $(SOURCES_LIB:.c=.o)
	"$(CC)" $(LDFLAGS) -dynamiclib -fvisibility=hidden -o $@ $^
	strip -u $(DYLIB)

%.o: %.c
	"$(CC)" $(CFLAGS) -fvisibility=hidden -c -o $@ $<

clean:
	rm -f rtos *.o Dependencies.mk $(DYLIB)

.PHONY: clean rtos

CC=clang
CFLAGS=-Os -fno-unwind-tables -fno-stack-protector -fomit-frame-pointer -fno-rtti -fno-exceptions -Wall -Werror -flto
LDFLAGS=-Os -Wall -Werror -flto

rtos: main.o lock.o rtos.o rtos_yield.o rtos_sched.o syscalls_data.o syscalls_exit.o syscalls_futex.o syscalls_sleep.o sys_auto.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f rtos *.o

.PHONY: clean

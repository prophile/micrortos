CC=clang
CFLAGS=-Os -fno-unwind-tables -fno-stack-protector -fomit-frame-pointer -fno-rtti -fno-exceptions -Wall -Werror -flto
LDFLAGS=-Os -Wall -Werror -flto

rtos: main.o rtos.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f rtos *.o

.PHONY: clean

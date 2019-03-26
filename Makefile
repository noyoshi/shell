CFLAGS=-Wall -o

all: program

program: shell.c debug.h
	gcc $(CFLAGS) shell shell.c

dev:
	mv config.h.in config.h
	gcc $(CFLAGS) shell shell.c
	mv config.h config.h.in

clean:
	@echo Removing binaries...
	rm myshell

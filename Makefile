CFLAGS=-Wall -o

all: program

program:
	gcc $(CFLAGS) shell shell.c

clean:
	@echo Removing binaries...
	rm myshell

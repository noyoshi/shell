CFLAGS=-Wall -o

all: program

program: shell.c debug.h 
	gcc $(CFLAGS) shell shell.c 

dev:
	gcc -D DEBUG $(CFLAGS) shell shell.c

clean:
	@echo Removing binaries...
	rm shell 

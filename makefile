CC=gcc
CFLAGS=-Wall

ANSI=-ansi

SRCMODULES=server.c
OBJMODULES=$(SRCMODULES:.c=.o)
HEDMODULES=$(SRCMODULES:.c=.h)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(ANSI) -g $< -c -o $@

main: main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $(ANSI) -g $^ -o $@
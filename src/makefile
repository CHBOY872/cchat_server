CC=gcc
CFLAGS=-Wall -g

ANSI=-ansi
ISOSTD=

SRCMODULES=server.c
HDRMODULES=$(SRCMODULES:.c=.h)
OBJMODULES=$(SRCMODULES:.c=.o)

%.o: %.c %.h
	$(CC) $(CFLAGS) $(ANSI) $< -c -o $@

main: main.c $(OBJMODULES)
	$(CC) $(CFLAGS) $(ANSI) $^ -o $@

ifneq (clean, $(MAKECMDGOALS))
-include deps.mk
endif

deps.mk: $(SRCMODULES)
	$(CC) -MM $^ > $@

clean: 
	rm *.o

all:
	make main
	make clean
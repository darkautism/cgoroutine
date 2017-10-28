MAKE = make
CC = gcc
CFLAGS = -std=gnu99 -Ilfqueue -I.
LDFLAGS = -lpthread -llfq -Llfqueue

default: liblfq
	$(CC) $(CFLAGS) example/yield_and_next.c cgoroutine.c $(LDFLAGS) 

liblfq: lfqueue/*
	$(MAKE) -C lfqueue
	
clean:
	rm -rf *.o bin/* liblfq.so.1.0.0 liblfq.a
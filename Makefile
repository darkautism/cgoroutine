MAKE = make
CC = gcc
CFLAGS = -std=gnu99 -Ilfqueue -I.
LDFLAGS = -lpthread -llfq -Llfqueue

default: liblfq
	$(CC) $(CFLAGS) example/yield.c cgoroutine.c $(LDFLAGS) -g -o test_yield

liblfq: lfqueue/*
	$(MAKE) -C lfqueue liblfq
	
clean:
	rm -rf *.o bin/* liblfq.so.1.0.0 liblfq.a
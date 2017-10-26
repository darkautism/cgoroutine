MAKE = make
CC = gcc
CFLAGS = -std=gnu99 -Ilfqueue
LDFLAGS = -lpthread -llfq -Llfqueue

default:
	$(CC) $(CFLAGS) yield_and_next.c cgoroutine.c $(LDFLAGS) 

liblfq: lfqueue/*
	$(MAKE) -C lfqueue liblfq
	
clean:
	rm -rf *.o bin/* liblfq.so.1.0.0 liblfq.a
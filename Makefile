CC=gcc
CFLAGS=--std=gnu99 -Wextra -g
vpath %.c src/
vpath %.h src/

test: test_main.c threaded_dispatch.o mqueue.o queue.o
	$(CC) $(CFLAGS) -o $@ $^ -lpthread

threaded_dispatch.o: threaded_dispatch.c queue.h
	$(CC) $(CFLAGS) -c $<

mqueue.o: mqueue.c mqueue.h queue.h
	$(CC) $(CFLAGS) -c $<

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	-rm test
	-rm *.o

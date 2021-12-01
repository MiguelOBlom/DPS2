CC=gcc
CFLAGS=-ggdb

all: peer tracker


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CLAGS)


peer: peer.c common.o
	$(CC) -o $@ $^ $(CFLAGS)

tracker: tracker.c common.o
	$(CC) -o $@ $^ $(CFLAGS)



clean:
	rm -f peer tracker common.o

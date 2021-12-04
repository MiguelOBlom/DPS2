CC=gcc
CFLAGS=-ggdb
DEPS=common.h list.h db.h

all: peer tracker


%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CLAGS)


peer: peer.c common.o
	$(CC) -o $@ $^ $(CFLAGS)

tracker: tracker.c common.o list.o
	$(CC) -o $@ $^ $(CFLAGS)



clean:
	rm -f peer tracker *.o

CC=gcc
CFLAGS=-ggdb
LIBS=-lsqlite3 -pthread
DEPS=common.h db.h

all: peer tracker

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

peer: peer.c common.o db.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

tracker: tracker.c common.o db.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f peer tracker *.o

CC=gcc
CFLAGS=-ggdb
LIBS=-lsqlite3 -pthread
DEPS=common.h db.h crc.h queue.h
INC=-I./sqlite3
LIB=-L./sqlite3


all: peer tracker

%.o: %.c $(DEPS)
	$(CC) $(LIB) $(INC) -c -o $@ $< $(CFLAGS) $(LIBS) 

peer: peer.c common.o db.o crc.o queue.o
	$(CC) $(LIB) $(INC) -o $@ $^ $(CFLAGS) $(LIBS)

tracker: tracker.c common.o db.o crc.o queue.o
	$(CC) $(LIB) $(INC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f peer tracker *.o *.db

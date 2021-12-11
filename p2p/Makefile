CC=gcc
CFLAGS=-ggdb
LIBS=-lsqlite3 -pthread
DEPS=common.h db.h crc.h queue.h peer.h
INC=-I./sqlite3
LIB=-L./sqlite3


all: libpeer.so tracker

%.o: %.c $(DEPS)
	$(CC) $(LIB) $(INC) -c -o $@ $< $(CFLAGS) $(LIBS) 

libpeer.so: peer.c common.o db.o crc.o queue.o $(DEPS)
	$(CC) $(LIB) $(INC) -o $@ $^ $(CFLAGS) $(LIBS) -shared -fPIC

tracker: tracker.c common.o db.o crc.o queue.o
	$(CC) $(LIB) $(INC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f libpeer.so tracker *.o *.db
	
	


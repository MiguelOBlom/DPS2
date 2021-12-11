CC=gcc
CXX=g++
CFLAGS = -Wall -Wextra -ggdb
CXXFLAGS = $(CFLAGS) -std=c++11
LIBS=
DEPS=sha256.h blockchain.h block.h iproofofwork.h hashcash.h 

all:	application

application: application.cc sha256.o
	$(CXX) -o $@ $^ $(CFLAGS) $(LIBS)

%.o:	%.cc $(DEPS)
	$(CXX) -c $< $(CXXFLAGS) $(LIBS)

clean:
	rm -f application *.o
	
	
	
CC=gcc
CFLAGS=-ggdb
LIBS=-lsqlite3 -pthread
DEPS=common.h db.h crc.h queue.h peer.h
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

CC=gcc
CXX=g++
CFLAGS=-Wall -Wextra -ggdb
CXXFLAGS=$(CFLAGS) -std=c++11
LIBS=-lsqlite3 -pthread
DEPS=common.h db.h crc.h queue.h peer.h 
CXXDEPS=sha256.h blockchain.h block.h iproofofwork.h hashcash.h 
INC=-I./sqlite3
LIB=-L./sqlite3


all: application tracker

peer.o: peer.c $(DEPS)
	gcc -L./sqlite3 -I./sqlite3 -c -o $@ $< $(CFLAGS) -lsqlite3 -pthread

common.o: common.c $(DEPS)
	gcc $(LIB) $(INC) -c -o $@ $< $(CFLAGS) $(LIBS) 

db.o: db.c $(DEPS)
	gcc -L./sqlite3 -I./sqlite3 -c -o $@ $< $(CFLAGS) -lsqlite3

crc.o: crc.c $(DEPS)
	gcc $(LIB) $(INC) -c -o $@ $< $(CFLAGS) $(LIBS) 

queue.o: queue.c $(DEPS)
	gcc $(LIB) $(INC) -c -o $@ $< $(CFLAGS) $(LIBS) 

sha256.o: sha256.cc $(CXXDEPS)
	g++ -c $< $(CXXFLAGS) $(CXXLIBS)

application: application.cc peer.o common.o db.o crc.o queue.o sha256.o $(DEPS) $(CXXDEPS)
	g++ $(LIB) $(INC) -o $@ $^ $(CFLAGS) $(CXXLIBS)

tracker: tracker.c common.o db.o crc.o queue.o
	gcc $(LIB) $(INC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f application tracker *.o *.db
	
	


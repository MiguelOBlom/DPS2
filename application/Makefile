# Authors: Miguel Blom and Matt van den Nieuwenhuijzen

CC=gcc
CXX=g++
CFLAGS=-Wall -Wextra -ggdb
CXXFLAGS=$(CFLAGS) -std=c++11
DEPS=sha256.h blockchain.h block.h iproofofwork.h hashcash.h 

all: builddir sha256/libsha256.so sqlite3/libsqlite3.so pow/libpow.so p2p/libpeer.so build/tracker blockchain/libblockchain.so build/application

builddir:
	mkdir -p build

application.o: application.cc application.h
	$(CXX) $(CXXFLAGS) -I./blockchain -I./sha256 -I./pow -I./p2p -c -o $@ $< -L./blockchain -L./sha256 -L./pow -L./p2p -lblockchain -lsha256 -lpow -lpeer

logger.o: logger.cc logger.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

build/application: main.cc application.o logger.o lock_vector.h transaction.h transaction_reader.h application.h 
	$(CXX) $(CXXFLAGS) -I./blockchain -I./sha256 -I./pow -I./p2p -o $@ $^ -L./blockchain -L./sha256 -L./pow -L./p2p -lblockchain -lsha256 -lpow -lpeer -pthread

sqlite3/libsqlite3.so: sqlite3/shell.c sqlite3/sqlite3.c sqlite3/sqlite3.h sqlite3/sqlite3ext.h
	$(CC) $(CFLAGS) -o $@ $^ -pthread -ldl -shared -fPIC

sha256/libsha256.so: sha256/sha256.cc sha256/sha256.h
	$(CXX) $(CXXFLAGS) -o $@ $^ -shared -fPIC

pow/libpow.so: pow/hashcash.cc pow/hashcash.h pow/iproofofwork.h
	$(CXX) $(CXXFLAGS) -I./sha256 -o $@ $^ -shared -fPIC -L./sha256 -lsha256 

p2p/common.o: p2p/common.c p2p/common.h p2p/crc.h
	$(CC) $(CFLAGS) -c -o $@ $< -fPIC
	
p2p/db.o: p2p/db.c p2p/db.h
	$(CC) $(CFLAGS) -c -o $@ $< -fPIC
	
p2p/crc.o: p2p/crc.c p2p/crc.h config.h
	$(CC) $(CFLAGS) -c -o $@ $< -fPIC
	
p2p/queue.o: p2p/queue.c p2p/queue.h
	$(CC) $(CFLAGS) -c -o $@ $< -fPIC

p2p/libpeer.so: p2p/peer.c p2p/common.o p2p/db.o p2p/crc.o p2p/queue.o p2p/common.h p2p/db.h p2p/crc.h p2p/queue.h p2p/peer.h
	$(CC) $(CFLAGS) -I./sqlite3 -o $@ $^ -shared -fPIC -lsqlite3 -pthread -L./sqlite3

build/tracker: p2p/tracker.c p2p/common.o p2p/db.o p2p/crc.o p2p/queue.o
	$(CC) $(CFLAGS) -I./sqlite3 -o $@ $^ -lsqlite3 -pthread -L./sqlite3

blockchain/libblockchain.so: blockchain/block.h blockchain/blockchain.h
	$(CXX) $(CXXFLAGS) -I./sqlite3 -I./sha256 -o $@ $^ -shared -fPIC -lsqlite3 -lsha256 -L./sqlite3 -L./sha256

clean:
	rm -f sha256/libsha256.so sqlite3/libsqlite3.so pow/libpow.so build/tracker p2p/*.o p2p/libpeer.so blockchain/libblockchain.so build/application application.o *.o

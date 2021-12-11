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

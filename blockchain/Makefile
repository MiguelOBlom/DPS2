CC=gcc
CXX=g++
CFLAGS = -Wall -Wextra -Wno-unused-result -Wno-unused-parameter -Wno-unused-but-set-variable -O3
CXXFLAGS = $(CFLAGS) -std=c++11
LIBS=
DEPS=sha256.h

all:	blockchain

blockchain: main.o sha256.o $(DEPS)
	$(CXX) $(CFLAGS) $(LIBS) -o $@ $^

%.o:	%.cc $(DEPS)
	$(CXX) $(CXXFLAGS) $(LIBS) -c $<

clean:
	rm -f blockchain *.o

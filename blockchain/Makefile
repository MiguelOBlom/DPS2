CC=gcc
CXX=g++
CFLAGS = -Wall -Wextra -Wno-unused-result -Wno-unused-parameter -Wno-unused-but-set-variable -O3
CXXFLAGS = $(CFLAGS) -std=c++11
LIBS=
DEPS=sha256.h

all:	blockchain

blockchain: main.o sha256.o
	$(CXX) -o $@ $^ $(CFLAGS) $(LIBS)

%.o:	%.cc $(DEPS)
	$(CXX) -c $< $(CXXFLAGS) $(LIBS)

clean:
	rm -f blockchain *.o

CC=gcc
CXX=g++
CFLAGS = -Wall -Wextra -Wno-unused-result -Wno-unused-parameter -Wno-unused-but-set-variable -O3 -lsha
CXXFLAGS = $(CFLAGS) -std=c++11

all:	blockchain

blockchain:	main.o	sha256.o
	$(CXX) $(CFLAGS) -o $@ $^

%.o:	%.cc
	$(CXX) $(CXXFLAGS) -c $<

clean:
	rm -f blockchain *.o

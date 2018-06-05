CXX=g++
CFLAGS=-std=c++11 -Wall -g

INCS=include
SRCS=src/UdpChat.cc

all: compile

compile: UdpChat.o
	$(CXX) -o UdpChat UdpChat.o

UdpChat.o:
	$(CXX) $(CFLAGS) -I $(INCS) -c $(SRCS)

clean:
	rm *.o UdpChat

CXX=g++
CFLAGS=-std=c++11 -Wall -g

INCS=include
SRCS=src/gbnnode.cc
OBJS=build/gbnnode.o
all: compile

compile: gbnnode.o
	$(CXX) -o gbnnode $(OBJS) -lpthread

gbnnode.o:
	$(CXX) $(CFLAGS) -I $(INCS) -c $(SRCS) -o build/gbnnode.o

clean:
	rm $(OBJS)

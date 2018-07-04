CXX=g++
CFLAGS=-std=c++11 -Wall -g

INCS=include
SRCS=src/gbnnode.cc
OBJS=build/gbnnode.o
all: compile

compile: gbnnode.o dvnode.o cnnode.o
	$(CXX) -o gbnnode $(OBJS) -lpthread
	$(CXX) -o dvnode build/dvnode.o -lpthread
	$(CXX) -o cnnode build/cnnode.o -lpthread

gbnnode.o:
	$(CXX) $(CFLAGS) -I $(INCS) -c $(SRCS) -o build/gbnnode.o

dvnode.o:
	$(CXX) $(CFLAGS) -I $(INCS) -c src/dvnode.cc -o build/dvnode.o

cnnode.o:
	$(CXX) $(CFLAGS) -I $(INCS) -c src/cnnode.cc -o build/cnnode.o

clean:
	rm $(OBJS)

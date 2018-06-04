CXX=g++
CFLAGS=-std=c++11 -Wall

INCS=include
SRCS=src/print_table.cc
DEST=build
ODIR=obj
all: compile

compile: print_table.o
	mkdir -p $(DEST)
	$(CXX) -o print_table print_table.o

print_table.o:
	$(CXX) $(CFLAGS) -I $(INCS) -c $(SRCS)

CXX=g++
CFLAGS=-std=c++11 -Wall

INCS=include
SRCS=src/print_table.cc

all: compile

compile: print_table.o
	$(CXX) -o print_table.out print_table.o

print_table.o:
	$(CXX) $(CFLAGS) -I $(INCS) -c $(SRCS)

clean:
	rm *.o *.out

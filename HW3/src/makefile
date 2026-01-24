CC = g++
CFlags = -std=c++11 -O3

all: main

main: main.o
	$(CC) $(CFlags) -o ../bin/hw3 main.o

main.o: main.cpp
	$(CC) $(CFlags) -c main.cpp

clean:
	rm -rf main.o
	rm -rf ../bin/hw3
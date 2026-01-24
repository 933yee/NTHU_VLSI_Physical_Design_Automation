CC = g++
CFlags = -std=c++17 -O3 -march=native -flto -DNDEBUG

all: main

main: main.o
	$(CC) $(CFlags) -o ../bin/hw5 main.o

main.o: main.cpp
	$(CC) $(CFlags) -c main.cpp

clean:
	rm -rf main.o
	rm -rf ../bin/hw5

CC = g++
CFlags = -O3 -std=c++11 -fopenmp

all: main main_parallel

main: main.o
	$(CC) $(CFlags) -o ../bin/hw2 main.o

main.o: main.cpp
	$(CC) $(CFlags) -c main.cpp

main_parallel: main_parallel.o
	$(CC) $(CFlags) -o ../bin/hw2_parallel main_parallel.o

main_parallel.o: main_parallel.cpp
	$(CC) $(CFlags) -c main_parallel.cpp

clean:
	rm -rf main.o main_parallel.o
	rm -rf ../bin/hw2 ../bin/hw2_parallel
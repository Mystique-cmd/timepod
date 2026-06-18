CC?=gcc
CXX?=g++
CFLAGS?=-O2 -Wall -Wextra -std=c11
CXXFLAGS?=-O2 -Wall -Wextra -std=c++17

all: timepod

timepod: main.o timer.o io.o
	$(CXX) -o $@ $^

main.o: main.cpp timer.h io.h
	$(CXX) $(CXXFLAGS) -c main.cpp

timer.o: timer.c timer.h
	$(CC) $(CFLAGS) -c timer.c


io.o: io.c io.h
	$(CC) $(CFLAGS) -c io.c

clean:
	rm -f *.o timepod



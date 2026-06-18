CC?=gcc
CXX?=g++
CFLAGS?=-O2 -Wall -Wextra -std=c11
CXXFLAGS?=-O2 -Wall -Wextra -std=c++17

all: timepod debug_run

timepod: main.o timer.o io.o ui.o terminal.o timer_nb.o

	$(CXX) -o $@ $^

debug_run: debug_run.o

	$(CC) -o $@ $^


main.o: main.cpp timer.h timer_nb.h io.h ui.h terminal.h

	$(CXX) $(CXXFLAGS) -c main.cpp

debug_run.o: debug_run.c
	$(CC) $(CFLAGS) -c debug_run.c


timer.o: timer.c timer.h
	$(CC) $(CFLAGS) -c timer.c

ui.o: ui.c ui.h timer.h
	$(CC) $(CFLAGS) -c ui.c

terminal.o: terminal.c terminal.h
	$(CC) $(CFLAGS) -c terminal.c

timer_nb.o: timer_nb.c timer_nb.h timer.h
	$(CC) $(CFLAGS) -c timer_nb.c


io.o: io.c io.h
	$(CC) $(CFLAGS) -c io.c


clean:
	rm -f *.o timepod debug_run timepod_days.bin





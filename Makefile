CC?=gcc
CXX?=g++
CFLAGS?=-O2 -Wall -Wextra -std=c11
CXXFLAGS?=-O2 -Wall -Wextra -std=c++17

all: timepod


# Build TUI-only binary only when TIMEPOD_ENABLE_TUI is enabled.
# Otherwise, compile a minimal non-interactive build.
ifeq ($(TIMEPOD_ENABLE_TUI),1)
timepod: main.o timer.o io.o ui.o terminal.o timer_nb.o session_store.o

	$(CXX) -o $@ $^
else
all: timepod

timepod: main.o timer.o ui.o timer_nb.o session_store.o

	$(CXX) -o $@ $^
endif



debug_run: debug_run.o

	$(CC) -o $@ $^


ifeq ($(TIMEPOD_ENABLE_TUI),1)
main.o: main.cpp timer.h timer_nb.h io.h ui.h terminal.h
else
main.o: main.cpp timer.h timer_nb.h ui.h
endif

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

task_intake.o: task_intake.c task_intake.h timer.h
	$(CC) $(CFLAGS) -c task_intake.c

task_store.o: task_store.c task_store.h task_intake.h
	$(CC) $(CFLAGS) -c task_store.c

io.o: io.c io.h
	$(CC) $(CFLAGS) -c io.c


clean:
	rm -f *.o timepod debug_run timepod_days.bin timepod_tasks.bin





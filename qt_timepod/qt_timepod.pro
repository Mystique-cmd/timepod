QT += widgets
CONFIG += c++17 console

SOURCES += main.cpp \
           ../ui.c \
           ../timer.c \
           ../timer_nb.c \
           ../terminal.c


HEADERS += ../ui.h ../timer.h ../timer_nb.h

INCLUDEPATH += ..

TARGET = timepod_qt



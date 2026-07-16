QT += widgets
CONFIG += c++17 console

SOURCES += main.cpp \
           ../ui.c \
           ../timer.c \
           ../timer_nb.c \
           ../notify.c \
           ../session_store.c

HEADERS += ../ui.h ../timer.h ../timer_nb.h ../notify.h ../session_store.h



INCLUDEPATH += ..

TARGET = timepod_qt




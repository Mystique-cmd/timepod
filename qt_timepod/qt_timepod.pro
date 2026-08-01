QT += widgets network
CONFIG += c++17 console

SOURCES += main.cpp \
           ai_client.cpp \
           ../ui.c \
           ../timer.c \
           ../timer_nb.c \
           ../notify.c \
           ../session_store.c \
           ../task_intake.c \
           ../task_store.c

HEADERS += ../ui.h ../timer.h ../timer_nb.h ../notify.h ../session_store.h \
           ../task_intake.h ../task_store.h \
           ai_client.h

INCLUDEPATH += ..

TARGET = timepod_qt




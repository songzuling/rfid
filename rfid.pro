TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    serial.c \
    rfid.c

HEADERS += \
    serial.h \
    rfid.h

LIBS += -lpthread

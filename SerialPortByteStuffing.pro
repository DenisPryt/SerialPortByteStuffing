#-------------------------------------------------
#
# Project created by QtCreator 2015-10-14T19:53:55
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SerialPortByteStuffing
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    Crc16.cpp

HEADERS  += MainWindow.h \
    Crc16.h

FORMS    += MainWindow.ui

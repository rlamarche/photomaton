#-------------------------------------------------
#
# Project created by QtCreator 2013-04-14T14:54:06
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = photomaton
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    pmgphotocommandthread.cpp

HEADERS  += mainwindow.h \
    pmgphotocommandthread.h

FORMS    += mainwindow.ui

LIBS     += -lgphoto2

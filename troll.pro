#-------------------------------------------------
#
# Project created by QtCreator 2016-10-12T11:19:32
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = troll
TEMPLATE = app


SOURCES +=\
        mainwindow.cxx \
	main.cxx \
    libtroll/libtroll.cxx \
    sforth/engine.c \
    sforth/sf-opt-file.c \
    sforth/sf-opt-prog-tools.c \
    sforth/sf-opt-string.c \
    sforth.cxx \
    cortexm0.cxx \
    cortexm0-sfext.c \
    target.cxx

HEADERS  += mainwindow.hxx \
    libtroll/dwarf.h \
    libtroll/libtroll.hxx \
    sforth.hxx \
    cortexm0.hxx \
    util.hxx \
    target.hxx

FORMS    += mainwindow.ui

INCLUDEPATH += libtroll/ sforth/

DEFINES += ENGINE_32BIT CORE_CELLS_COUNT="128*1024" STACK_DEPTH=32

DISTFILES += \
    unwinder.fs

RESOURCES += \
    resources.qrc

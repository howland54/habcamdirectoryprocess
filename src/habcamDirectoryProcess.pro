#-------------------------------------------------
#
# Project created by QtCreator 2015-11-12T19:19:15
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = habcamDirectoryProcess
CONFIG   += console
CONFIG   -= app_bundle
INCLUDEPATH += /usr/include/opencv4/

TEMPLATE = app
LIBS += -L/usr/lib/x86_64-linux-gnu/\
   -lopencv_core\
   -lopencv_highgui\
   -lopencv_imgproc\
   -lopencv_features2d\
   -lopencv_imgcodecs\
   -lopencv_calib3d\
   -ltiff\
   -lexiv2


SOURCES += main.cpp

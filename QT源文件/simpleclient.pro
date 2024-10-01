QT += core gui network mqtt axcontainer network multimedia multimediawidgets
QT += multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

#TARGET = simplemqttclient
#TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp\
        mainwindow.cpp 

HEADERS  += mainwindow.h 
FORMS    += mainwindow.ui


#target.path = $$[QT_INSTALL_EXAMPLES]/mqtt/simpleclient
#INSTALLS += target

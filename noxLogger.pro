QT       += core gui
QT += serialport
QT += svgwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += qwt

DEFINES += QWT_DLL

VERSION = 1.0.0
DEFINES += PROGRAM_VERSION=\\\"$$VERSION\\\"

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include (C:/Users/simonenp/Documents/QtProjects/Qwt-6.2.0/features/qwt.prf)

SOURCES += \
    aboutwindow.cpp \
    devicedata.cpp \
    logwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    oqfiledialog.cpp \
    oqpanner.cpp \
    oqplot.cpp \
    oqplotcurve.cpp \
    oqplotpicker.cpp \
    oqserialport.cpp \
    quickguidewindow.cpp \
    serialcontroller.cpp \
    statuswindow.cpp

HEADERS += \
    aboutwindow.h \
    devicedata.h \
    logwindow.h \
    mainwindow.h \
    oqfiledialog.h \
    oqpanner.h \
    oqplot.h \
    oqplotcurve.h \
    oqplotpicker.h \
    oqserialport.h \
    quickguidewindow.h \
    serialcontroller.h \
    statuswindow.h

FORMS += \
    aboutwindow.ui \
    logwindow.ui \
    mainwindow.ui \
    quickguidewindow.ui \
    statuswindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc

RC_ICONS = images/noxIcon.ico

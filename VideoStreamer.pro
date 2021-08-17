QT += core gui qml widgets quick

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        datagram.cpp \
        framecontainer.cpp \
        iservicetcpsocket.cpp \
        ivideoupdsocket.cpp \
        main.cpp \
        mainwindow.cpp \
        servicetcpclient.cpp \
        servicetcpserver.cpp \
        socketthread.cpp \
        videoclientsettings.cpp \
        videostreamcontroller.cpp \
        videostreamer.cpp \
        videostreamsettings.cpp \
        videoudpclient.cpp \
        videoudpserver.cpp

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

LIBS += -lavutil
LIBS += -lavcodec

INCLUDEPATH += $${SRC_DIR}/sipcore/services
INCLUDEPATH += $${SRC_DIR}/sipcore/services/include

HEADERS += \
    datagram.h \
    framecontainer.h \
    iservicetcpsocket.h \
    ivideoupdsocket.h \
    mainwindow.h \
    servicetcpclient.h \
    servicetcpserver.h \
    socketthread.h \
    videoclientsettings.h \
    videostreamcontroller.h \
    videostreamer.h \
    videostreamsettings.h \
    videoudpclient.h \
    videoudpserver.h

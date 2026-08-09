QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ../../../Src/Drivers/AudioDriver.h \
    ../../../Src/Drivers/BluetoothDriver.h \
    ../../../Src/Drivers/DisplayDriver.h \
    ../../../Src/Drivers/IMUDriver.h \
    ../../../Src/Drivers/SDCardDriver.h \
    ../../../Src/HAL/Conversion.h \
    ../../../Src/HAL/HAL.h \
    ../../../Src/Src/AudioPlayer.h \
    ../../../Src/Src/Button.h \
    ../../../Src/Src/Elapsed.h \
    ../../../Src/Src/FileManager.h \
    ../../../Src/Src/Log.h \
    ../../../Src/Src/MusicManager.h \
    ../../../Src/Src/Pedometer.h \
    ../../../Src/Src/Song.h \
    ../../../Src/Src/System.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

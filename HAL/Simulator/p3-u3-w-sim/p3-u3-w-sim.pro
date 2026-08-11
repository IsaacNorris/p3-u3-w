QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# The simulator's SD card is this folder, so test content survives switching
# between Debug and Release or wiping the build directory. shell_quote keeps it
# intact when the checkout path contains spaces.
DEFINES += SD_CARD_ROOT=$$shell_quote(\"$$PWD/sdcard\")

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../../../Src/Src/Decoders/Mp3Decoder.cpp \
    AudioDriver.cpp \
    SDCardDriver.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    ../../../ThirdParty/minimp3/minimp3.h \
    ../../../Src/Drivers/AudioDriver.h \
    ../../../Src/Drivers/BluetoothDriver.h \
    ../../../Src/Drivers/DisplayDriver.h \
    ../../../Src/Drivers/IMUDriver.h \
    ../../../Src/Drivers/SDCardDriver.h \
    ../../../Src/HAL/Conversion.h \
    ../../../Src/HAL/HAL.h \
    ../../../Src/Src/AudioFormat.h \
    ../../../Src/Src/AudioPlayer.h \
    ../../../Src/Src/Button.h \
    ../../../Src/Src/Decoders/AudioDecoder.h \
    ../../../Src/Src/Decoders/Mp3Decoder.h \
    ../../../Src/Src/Decoders/WavDecoder.h \
    ../../../Src/Src/Display.h \
    ../../../Src/Src/Elapsed.h \
    ../../../Src/Src/File.h \
    ../../../Src/Src/FileManager.h \
    ../../../Src/Src/Log.h \
    ../../../Src/Src/MusicManager.h \
    ../../../Src/Src/Pedometer.h \
    ../../../Src/Src/Song.h \
    ../../../Src/Src/StreamBuffer.h \
    ../../../Src/Src/System.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

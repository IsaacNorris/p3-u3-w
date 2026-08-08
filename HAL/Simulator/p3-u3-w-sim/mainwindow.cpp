#include "mainwindow.h"

#include "ui_mainwindow.h"

#include "../../../Src/Src/System.h"
#include "../../../Src/Hal/HAL.h"

MainWindow* mainWindow;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    systemTimerElapsed.start();
    mainWindow = this;

    tSystem* system = new tSystem();

    loopTimer.setTimerType(Qt::PreciseTimer);
    loopTimer.setInterval(1);
    QObject::connect(&loopTimer, &QTimer::timeout, this, [=]() {
        system->Update();
    });

    loopTimer.start();
}

MainWindow::~MainWindow() { delete ui; }

tDigitalValue HAL::ReadDigitalInput(HAL::eDigitalInput pin){
    tDigitalValue value {};

    switch(pin){
        case HAL::eDigitalInput::ButtonPlay:
            value = mainWindow->ui->pb_pausePlay->isDown();
            break;
        case HAL::eDigitalInput::ButtonMenu:
            value = mainWindow->ui->pb_menu->isDown();
            break;
        case HAL::eDigitalInput::ButtonSkip:
            value = mainWindow->ui->pb_Skip->isDown();
            break;
        case HAL::eDigitalInput::ButtonVolumeUp:
            value = mainWindow->ui->pb_volP->isDown();
            break;
        case HAL::eDigitalInput::ButtonVolumeDown:
            value = mainWindow->ui->pb_volM->isDown();
            break;
    }

    return value;
}

tVoltage HAL::ReadAnalogInput(HAL::eAnalogInput pin){
    tVoltage voltage {};

    switch (pin) {
        case HAL::eAnalogInput::BatteryPercentage:
            // voltage = 0.0;
            voltage = 3.3;
            break;
    }

    return voltage;
}

tTimeMs HAL::GetCurrentTimeMs(){
    return mainWindow->systemTimerElapsed.elapsed();
}

void HAL::Print(const char* str){
    qDebug() << str;
}

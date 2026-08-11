#include "mainwindow.h"

#include "../../../Src/Hal/HAL.h"
#include "../../../Src/Src/System.h"
#include "ui_mainwindow.h"


MainWindow* mainWindow;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    systemTimerElapsed.start();
    mainWindow = this;

    system_ = std::make_unique<tSystem>();

    loopTimer.setTimerType(Qt::PreciseTimer);
    loopTimer.setInterval(1);
    QObject::connect(&loopTimer, &QTimer::timeout, this,
                     [this]() { system_->Update(); });

    loopTimer.start();
}

MainWindow::~MainWindow() {
    // The tick reaches into the widgets, so it has to be off before any of this
    // is torn down.
    loopTimer.stop();
    system_.reset();
    delete ui;
}

tDigitalValue HAL::ReadDigitalInput(HAL::eDigitalInput pin) {
    tDigitalValue value{};

    switch (pin) {
        case HAL::eDigitalInput::ButtonPlay:
            value = mainWindow->ui->pb_pausePlay->isDown();
            break;
        case HAL::eDigitalInput::ButtonMenu:
            value = mainWindow->ui->pb_menu->isDown();
            break;
        case HAL::eDigitalInput::ButtonNext:
            value = mainWindow->ui->pb_next->isDown();
            break;
        case HAL::eDigitalInput::ButtonPrevious:
            value = mainWindow->ui->pb_previous->isDown();
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

tVoltage HAL::ReadAnalogInput(HAL::eAnalogInput pin) {
    tVoltage voltage{};

    switch (pin) {
        case HAL::eAnalogInput::BatteryPercentage:
            voltage = mainWindow->ui->s_battery->value() / 3;
            voltage /= 10;
            // qDebug() << voltage;
            break;
    }

    return voltage;
}

tTimeMs HAL::GetCurrentTimeMs() {
    return mainWindow->systemTimerElapsed.elapsed();
}

void HAL::Print(const char* str) { qDebug() << str; }

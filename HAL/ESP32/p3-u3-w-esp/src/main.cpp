#include <Arduino.h>
#include <esp_system.h>

#include "../../../../Src/HAL/HAL.h"
#include "../../../../Src/Src/System.h"
#include "BoardPins.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

// mp3dec_decode_frame stages about 17 KB on the stack of whoever calls
// tAudioPlayer::Update, which is this task, and the Arduino default is 8 KB.
SET_LOOP_TASK_STACK_SIZE(32 * 1024);

tSystem* __system;

namespace {

// Survives the reset that caused it, which is the point: a rail that dips far
// enough to reset the board also dips below what the SD card needs, and a
// brownout is the one reason here that cannot be reported as it happens.
const char* ResetReason() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:
            return "power on";
        case ESP_RST_EXT:
            return "external reset";
        case ESP_RST_SW:
            return "software reset";
        case ESP_RST_PANIC:
            return "panic";
        case ESP_RST_INT_WDT:
            return "interrupt watchdog";
        case ESP_RST_TASK_WDT:
            return "task watchdog";
        case ESP_RST_WDT:
            return "watchdog";
        case ESP_RST_DEEPSLEEP:
            return "deep sleep wake";
        case ESP_RST_BROWNOUT:
            return "BROWNOUT, the 3.3 V rail went under";
        case ESP_RST_SDIO:
            return "SDIO";
        case ESP_RST_UNKNOWN:
            break;
    }
    return "unknown";
}

}  // namespace

void setup() {
    // An SD card drawing current from USB 5 V can dip 3.3 V hard enough for the
    // brownout detector to reset in a loop. On this board that makes the
    // USB-UART chip drop the COM port, so the serial monitor cannot open, which
    // is why the detector is normally off.
    //
    // Building with -D BROWNOUT_DETECT puts it back. Leave that off unless you
    // are chasing a rail sag: the detector trips at about 2.7 V, the card needs
    // about the same, and the inrush of SD.begin is enough to reset the board
    // and leave the card wedged so the next boot fails the mount.
#ifndef BROWNOUT_DETECT
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
#endif

    Serial.begin(115200);
    delay(500);
    Serial.println();
    Serial.println("p3-u3-w boot");
    Serial.print("Last reset: ");
    Serial.println(ResetReason());

    pinMode(BoardPins::SdChipSelect, OUTPUT);
    digitalWrite(BoardPins::SdChipSelect, HIGH);
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);

    pinMode(BoardPins::ButtonPlay, INPUT);
    pinMode(BoardPins::ButtonNext, INPUT);
    pinMode(BoardPins::ButtonPrevious, INPUT);
    pinMode(BoardPins::ButtonMenu, INPUT);
    pinMode(BoardPins::ButtonVolumeUp, INPUT);
    pinMode(BoardPins::ButtonVolumeDown, INPUT);

    __system = new tSystem();
}

void loop() { __system->Update(); }

tTimeMs HAL::GetCurrentTimeMs() {
    return millis();  // Return the number of milliseconds since the program
                      // started
}

void HAL::Print(const char* str) {
    if (str != nullptr) {
        Serial.print(str);
    }
}

tVoltage HAL::ReadAnalogInput(HAL::eAnalogInput pin) {
    tVoltage value{};

    switch (pin) {
        case HAL::eAnalogInput::BatteryPercentage:
            value = 2.0;  // Placeholder value for battery percentage
            break;
    }

    return value;
}

tDigitalValue HAL::ReadDigitalInput(HAL::eDigitalInput pin) {
    tDigitalValue value{};

    switch (pin) {
        case HAL::eDigitalInput::ButtonPlay:
            value = digitalRead(BoardPins::ButtonPlay) == HIGH;
            break;
        case HAL::eDigitalInput::ButtonNext:
            value = digitalRead(BoardPins::ButtonNext) == HIGH;
            break;
        case HAL::eDigitalInput::ButtonPrevious:
            value = digitalRead(BoardPins::ButtonPrevious) == HIGH;
            break;
        case HAL::eDigitalInput::ButtonMenu:
            value = digitalRead(BoardPins::ButtonMenu) == HIGH;
            break;
        case HAL::eDigitalInput::ButtonVolumeUp:
            value = digitalRead(BoardPins::ButtonVolumeUp) == HIGH;
            break;
        case HAL::eDigitalInput::ButtonVolumeDown:
            value = digitalRead(BoardPins::ButtonVolumeDown) == HIGH;
            break;
    }

    return value;
}
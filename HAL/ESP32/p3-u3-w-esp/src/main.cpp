#include <Arduino.h>

#include "../../../../Src/HAL/HAL.h"
#include "../../../../Src/Src/System.h"

tSystem* __system;

void setup() { __system = new tSystem(); }

void loop() { __system->Update(); }

tTimeMs HAL::GetCurrentTimeMs() {
    return millis();  // Return the number of milliseconds since the program
                      // started
}

void HAL::Print(const char* str) { Serial.println(str); }

tVoltage HAL::ReadAnalogInput(HAL::eAnalogInput pin) {
    tVoltage value{};

    switch (pin) {
        case HAL::eAnalogInput::BatteryPercentage:
            value = 2.5;  // Placeholder value for battery percentage
            break;
    }

    return value;
}

tDigitalValue HAL::ReadDigitalInput(HAL::eDigitalInput pin) {
    tDigitalValue value{};

    switch (pin) {
        case HAL::eDigitalInput::ButtonPlay:
            value = digitalRead(4) == HIGH;  // Example pin for ButtonPlay
            break;
        case HAL::eDigitalInput::ButtonNext:
            value = digitalRead(2) == HIGH;  // Example pin for ButtonNext
            break;
        case HAL::eDigitalInput::ButtonPrevious:
            value = digitalRead(0) == HIGH;  // Example pin for ButtonPrevious
            break;
        case HAL::eDigitalInput::ButtonMenu:
            value = digitalRead(15) == HIGH;  // Example pin for ButtonMenu
            break;
        case HAL::eDigitalInput::ButtonVolumeUp:
            value = digitalRead(16) == HIGH;  // Example pin for ButtonVolumeUp
            break;
        case HAL::eDigitalInput::ButtonVolumeDown:
            value =
                digitalRead(17) == HIGH;  // Example pin for ButtonVolumeDown
            break;
    }

    return value;
}
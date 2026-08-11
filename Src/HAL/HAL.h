#pragma once

#include <cstdint>

using tTimeMs = uint32_t;
using tVoltage = float;
using tDigitalValue = bool;
using tBatteryPercentage = uint8_t;
using tVolumeValue = uint8_t;
using tStepCount = uint32_t;

class HAL {
   public:
    enum class eDigitalInput {
        ButtonPlay,
        ButtonNext,
        ButtonPrevious,
        ButtonMenu,
        ButtonVolumeUp,
        ButtonVolumeDown,
    };

    enum class eAnalogInput {
        BatteryPercentage,
    };

    static tDigitalValue ReadDigitalInput(eDigitalInput pin);
    static tVoltage ReadAnalogInput(eAnalogInput pin);
    static tTimeMs GetCurrentTimeMs();
    static void Print(const char* str);
};

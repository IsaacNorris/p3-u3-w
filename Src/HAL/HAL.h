#pragma once

#include <cstdint>

using tTimeMs = uint32_t;
using tVoltage = float;
using tDigitalValue = bool;

class HAL {
   public:
    enum class eDigitalInput {
        ButtonPlay,
        ButtonSkip,
        ButtonMenu,
        ButtonVolumeUp,
        ButtonVolumeDown,
    };

    enum class eDigitalOutput {
        Heartbeat,
    };

    enum class eAnalogInput {
        BatteryPercentage,
    };

    static tDigitalValue ReadDigitalInput(eDigitalInput pin);
    static void WriteDigitalOutput(eDigitalOutput pin, tDigitalValue value);
    static tVoltage ReadAnalogInput(eAnalogInput pin);
    static tTimeMs GetCurrentTimeMs();
    static void Print(const char* str);
};
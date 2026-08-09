#pragma once

#include <cstdint>

using tTimeMs = uint32_t;
using tVoltage = float;
using tDigitalValue = bool;
using tBatteryPercentage = uint8_t;
using tVolumeValue = uint8_t;

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

    // SD card is always addressed in 512-byte sectors at this layer.
    // Simulator: back with a disk image or host FS bridge.
    // MCU: SPI/SDIO + card init.
    static constexpr uint16_t SdSectorSize = 512;

    static tDigitalValue ReadDigitalInput(eDigitalInput pin);
    static tVoltage ReadAnalogInput(eAnalogInput pin);
    static tTimeMs GetCurrentTimeMs();
    static void Print(const char* str);

    // Raw SD block device — platform-specific. Returns false on failure.
    static bool SdInit();
    static void SdDeinit();
    static bool SdIsPresent();
    static bool SdReadSectors(uint32_t startLba, void* buffer,
                              uint32_t sectorCount);
    static bool SdWriteSectors(uint32_t startLba, const void* buffer,
                               uint32_t sectorCount);
};

#pragma once

#include <cstdint>

// Freenove ESP32-WROOM (FNK0090 / ESP32-WROOM-32E) GPIO map. Onboard hardware
// owns 0 (BOOT), 2 (LED), and 16 (WS2812), so those stay off the buses and
// the buttons.

namespace BoardPins {

constexpr uint8_t I2cSda = 21;
constexpr uint8_t I2cScl = 22;

constexpr uint8_t SpiSck = 18;
constexpr uint8_t SpiMiso = 19;
constexpr uint8_t SpiMosi = 23;
constexpr uint8_t SdChipSelect = 32;

// GY-PCM5102 I2S: BCK, LCK/LRCK, DIN. SCK stays on GND. Keep XSMT jumpered
// to A3V3 (not an ESP32 GPIO — GPIO 5 is VSPI's default CS). FMT/DEMP/FLT
// stay low.
constexpr int I2sBitClock = 26;
constexpr int I2sWordSelect = 25;
constexpr int I2sDataOut = 27;

constexpr uint8_t ButtonPlay = 4;
constexpr uint8_t ButtonNext = 13;
constexpr uint8_t ButtonPrevious = 14;
constexpr uint8_t ButtonMenu = 15;
constexpr uint8_t ButtonVolumeUp = 33;
constexpr uint8_t ButtonVolumeDown = 17;

}  // namespace BoardPins

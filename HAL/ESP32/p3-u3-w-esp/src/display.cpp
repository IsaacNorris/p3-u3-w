#include "../../../../Src/Src/Display.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include <cstdio>
#include <cstring>

#include "../../../../Src/HAL/HAL.h"
#include "../../../../Src/Src/Log.h"
#include "BoardPins.h"

// ESP32 backing for the display: a 128x32 SSD1306 OLED on the Freenove
// ESP32-WROOM I2C pins. Every screen is text only for now, so each of the
// calls below composes what it was handed into a couple of lines and hands
// that to the one renderer.

namespace {

constexpr uint8_t ScreenWidth = 128;
constexpr uint8_t ScreenHeight = 32;
constexpr uint8_t I2cAddress = 0x3C;  // 0x3D on some modules

// At text size 1 a glyph is 6x8, so the panel holds 21 characters across and
// four lines down. The bottom line is kept for the status, leaving three for
// whatever the caller passed, and anything longer than that is cut.
constexpr size_t LineChars = 21;
constexpr size_t BodyChars = LineChars * 3;
constexpr int16_t StatusLineY = 24;

Adafruit_SSD1306 panel(ScreenWidth, ScreenHeight, &Wire, -1);

enum class eState : uint8_t {
    Untried,
    Ready,
    // Nothing answered on the bus. Retrying every tick would cost the superloop
    // an I2C timeout each time, so the panel is given up on instead.
    Missing,
};

eState state = eState::Untried;

// What is already on the panel. The screens are redrawn from the superloop on
// every tick, and pushing 512 bytes over I2C takes about 13 ms, which is a
// third of the audio ring. So a screen that has not changed is not sent.
char onScreen[BodyChars + LineChars + 2] = {};

bool Ready() {
    switch (state) {
        case eState::Ready:
            return true;
        case eState::Missing:
            return false;
        case eState::Untried:
            break;
    }

    Wire.begin(BoardPins::I2cSda, BoardPins::I2cScl);
    Wire.setTimeOut(50);
    if (!panel.begin(SSD1306_SWITCHCAPVCC, I2cAddress)) {
        Log::Error("No SSD1306 display answered on the I2C bus");
        state = eState::Missing;
        return false;
    }

    panel.setTextSize(1);
    panel.setTextColor(SSD1306_WHITE);

    state = eState::Ready;
    return true;
}

// status is optional and always lands on the bottom line, so it stays put
// however many lines the body above it takes.
void Show(const char* body, const char* status) {
    if (!Ready()) {
        return;
    }

    char text[BodyChars + 1];
    std::snprintf(text, sizeof(text), "%s", (body == nullptr) ? "" : body);

    char line[LineChars + 1];
    std::snprintf(line, sizeof(line), "%s", (status == nullptr) ? "" : status);

    char wanted[sizeof(onScreen)];
    std::snprintf(wanted, sizeof(wanted), "%s\n%s", text, line);
    if (std::strcmp(wanted, onScreen) == 0) {
        return;
    }
    std::strcpy(onScreen, wanted);

    panel.clearDisplay();

    panel.setCursor(0, 0);
    panel.print(text);

    if (status != nullptr) {
        panel.setCursor(0, StatusLineY);
        panel.print(line);
    }

    panel.display();
}

void ShowStatus(const char* body, tBatteryPercentage batteryPercentage,
                tVolumeValue volume) {
    char status[LineChars + 1];
    std::snprintf(status, sizeof(status), "Bat %u%%  Vol %u",
                  static_cast<unsigned>(batteryPercentage),
                  static_cast<unsigned>(volume));
    Show(body, status);
}

}  // namespace

void tDisplay::DisplayMusicPlayingScreen(const char* songName,
                                         tBatteryPercentage batteryPercentage,
                                         tVolumeValue volume) {
    ShowStatus(songName, batteryPercentage, volume);
}

void tDisplay::DisplayPlaylistScreen(const char* playlistName,
                                     tBatteryPercentage batteryPercentage,
                                     tVolumeValue volume) {
    ShowStatus(playlistName, batteryPercentage, volume);
}

void tDisplay::DisplayInitialisationScreen() { Show("Starting up", nullptr); }

void tDisplay::DisplayMainMenuScreen() { Show("Main Menu", nullptr); }

void tDisplay::DisplaySettingsScreen() { Show("Settings", nullptr); }

void tDisplay::DisplayPedometerScreen(tStepCount steps) {
    char body[LineChars + 1];
    std::snprintf(body, sizeof(body), "Steps %lu",
                  static_cast<unsigned long>(steps));
    Show(body, nullptr);
}

void tDisplay::DisplaySleepScreen() { Show("Sleeping", nullptr); }

void tDisplay::DisplayErrorScreen() { Show("Error", nullptr); }

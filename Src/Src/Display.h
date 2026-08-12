#pragma once

#include "../HAL/Conversion.h"
#include "../HAL/HAL.h"
#include "Log.h"
#include "Song.h"


class tDisplay {
   public:
    tDisplay();

    void DisplayMusicPlayingScreen(const char* songName,
                                   tBatteryPercentage batteryPercentage,
                                   tVolumeValue volume);
    void DisplayPlaylistScreen(const char* playlistName,
                               tBatteryPercentage batteryPercentage,
                               tVolumeValue volume);
    void DisplayInitialisationScreen();
    void DisplayMainMenuScreen();
    void DisplaySettingsScreen();
    void DisplayPedometerScreen(tStepCount steps);
    void DisplaySleepScreen();
    void DisplayErrorScreen();

   private:
};

inline tDisplay::tDisplay() {}

// //TODO: implement this.

// #warning qdebug enabled shouldn't be in hardware imple
// #include <QDebug>
// inline void tDisplay::DisplayMusicPlayingScreen(const char* songName,
// tBatteryPercentage batteryPercentage, tVolumeValue volume) {
//     qDebug() << "bat:" << batteryPercentage;
//     qDebug() << "vol:" << volume;
//     Log::Raw(songName);
// }

// inline void tDisplay::DisplayPlaylistScreen(const char* playlistName,
// tBatteryPercentage batteryPercentage, tVolumeValue volume) {
//     qDebug() << "bat:" << batteryPercentage;
//     qDebug() << "vol:" << volume;
//     Log::Raw(playlistName);
// }

// inline void tDisplay::DisplayInitialisationScreen() {}

// inline void tDisplay::DisplayMainMenuScreen() {}

// inline void tDisplay::DisplaySettingsScreen() {}

// inline void tDisplay::DisplayPedometerScreen(tStepCount steps) {}

// inline void tDisplay::DisplaySleepScreen() {}

// inline void tDisplay::DisplayErrorScreen() {}

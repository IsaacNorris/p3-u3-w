#pragma once

#include "../HAL/HAL.h"
#include "../HAL/Conversion.h"
#include "Song.h"
#include "../Drivers/DisplayDriver.h"

class tDisplay{
   public:
    tDisplay();

    void DisplayMusicPlayingScreen(const char* songName, tBatteryPercentage batteryPercentage, tVolumeValue volume);
    void DisplayPlaylistScreen(const char* playlistName, tBatteryPercentage batteryPercentage, tVolumeValue volume);
    void DisplayInitialisationScreen();
    void DisplayMainMenuScreen();
    void DisplaySettingsScreen();
    void DisplayPedometerScreen(tStepCount steps);
    void DisplaySleepScreen();
    void DisplayErrorScreen();

   private:
    tDisplayDriver displayDriver_;

};

inline tDisplay::tDisplay(){

}

//TODO: implement this.
inline void tDisplay::DisplayMusicPlayingScreen(const char* songName, tBatteryPercentage batteryPercentage, tVolumeValue volume) {}
inline void tDisplay::DisplayPlaylistScreen(const char* playlistName, tBatteryPercentage batteryPercentage, tVolumeValue volume) {}
inline void tDisplay::DisplayInitialisationScreen() {}
inline void tDisplay::DisplayMainMenuScreen() {}
inline void tDisplay::DisplaySettingsScreen() {}
inline void tDisplay::DisplayPedometerScreen(tStepCount steps) {}
inline void tDisplay::DisplaySleepScreen() {}
inline void tDisplay::DisplayErrorScreen() {}

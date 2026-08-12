#include "../../../../Src/Src/Display.h"

#include "../../../../Src/HAL/HAL.h"
#include "Wire.h"

void tDisplay::DisplayMusicPlayingScreen(const char* songName,
                                         tBatteryPercentage batteryPercentage,
                                         tVolumeValue volume) {}
void tDisplay::DisplayPlaylistScreen(const char* playlistName,
                                     tBatteryPercentage batteryPercentage,
                                     tVolumeValue volume) {}
void tDisplay::DisplayInitialisationScreen() {}
void tDisplay::DisplayMainMenuScreen() {}
void tDisplay::DisplaySettingsScreen() {}
void tDisplay::DisplayPedometerScreen(tStepCount steps) {}
void tDisplay::DisplaySleepScreen() {}
void tDisplay::DisplayErrorScreen() {}
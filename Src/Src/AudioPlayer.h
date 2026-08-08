#pragma once

#include "../Drivers/AudioDriver.h"
#include "../HAL/HAL.h"
#include "Log.h"

class tAudioPlayer {
   public:
    tAudioPlayer();

    void PlayPause();
    void IncrementVolume();
    void DecrementVolume();
    void NextSong();
    void PreviousSong();
   private:
};

inline tAudioPlayer::tAudioPlayer() {}

inline void tAudioPlayer::PlayPause() {}
inline void tAudioPlayer::IncrementVolume() {}
inline void tAudioPlayer::DecrementVolume() {}
inline void tAudioPlayer::NextSong() {}
inline void tAudioPlayer::PreviousSong() {}

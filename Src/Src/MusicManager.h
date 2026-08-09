#pragma once

#include "AudioPlayer.h"

class tMusicManager {
   public:
    tMusicManager();

    void Update();
    void IncrementVolume();
    void DecrementVolume();
    void NextSong();
    void PreviousSong();
    void PausePlaySong();

    void NextPlaylist();

    void SetVolumeStep(tVolumeValue step);

   private:
    tAudioPlayer audioPlayer_;
    tVolume volume_;
    tVolumeValue volumeStep_{5};
};

inline tMusicManager::tMusicManager() {}

inline void tMusicManager::Update() {}

inline void tMusicManager::IncrementVolume() { volume_ + volumeStep_; }

inline void tMusicManager::DecrementVolume() { volume_ - volumeStep_; }

inline void tMusicManager::SetVolumeStep(tVolumeValue step) {
    if (step >= 100) {
        step = 100;
    }

    volumeStep_ = step;
}

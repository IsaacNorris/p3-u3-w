#pragma once

#include "AudioPlayer.h"
#include "FileManager.h"
#include "Song.h"

#include <vector>

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
    std::vector<tPlaylist*> playlists_;

    tVolume volume_;
    tVolumeValue volumeStep_{5};

    void BuildPlaylists();
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

// we gotta be careful with the dynamic allocation.
inline void tMusicManager::BuildPlaylists(){
    playlists_.clear();
    auto folders = fileManager.GetFolders();
    for(auto folder : folders){
        tPlaylist* p = new tPlaylist(folder);
        playlists_.push_back(p);
    }
}

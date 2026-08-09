#pragma once

#include "AudioPlayer.h"
#include "FileManager.h"
#include "Song.h"

#include <vector>

class tMusicManager {
   public:
    tMusicManager();

    void Update();

    tPlaylist* CurrentPlaylist();
    tSong* CurrentSong();

    void IncrementVolume();
    void DecrementVolume();
    void NextSong();
    void PreviousSong();
    void PausePlaySong();
    void NextPlaylist();
    void PreviousPlaylist();

    void SetVolumeStep(tVolumeValue step);

   private:
    tAudioPlayer audioPlayer_;
    std::vector<tPlaylist*> playlists_;

    std::vector<tPlaylist*>::iterator currentPlaylistItr_ {playlists_.end()};

    tVolume volume_;
    tVolumeValue volumeStep_{5};

    bool isPlaying_ {false};

    void BuildPlaylists();
};

inline tMusicManager::tMusicManager() {
    // currentPlaylistItr_ = playlists_.end();
}

inline void tMusicManager::Update() {
    //where we will put the actual interface to the audio
    // i.e. the WRITE
}

inline tPlaylist* tMusicManager::CurrentPlaylist(){
    return *currentPlaylistItr_;
}

inline tSong* tMusicManager::CurrentSong(){
    tPlaylist* playlist = *currentPlaylistItr_;
    return playlist->CurrentSong();
}

inline void tMusicManager::IncrementVolume() { volume_ + volumeStep_; }

inline void tMusicManager::DecrementVolume() { volume_ - volumeStep_; }

inline void tMusicManager::NextSong(){
    tPlaylist* playlist = *currentPlaylistItr_;
    playlist->NextSong();
}

inline void tMusicManager::PreviousSong(){
    tPlaylist* playlist = *currentPlaylistItr_;
    playlist->PreviousSong();
}

inline void tMusicManager::PausePlaySong() { isPlaying_ = !isPlaying_; }

inline void tMusicManager::NextPlaylist(){
    if(currentPlaylistItr_ == playlists_.end()){
        Log::Error("Current Playlist Iterator invalid");
        return;
    }

    ++currentPlaylistItr_;

    if(currentPlaylistItr_ == playlists_.end()){
        currentPlaylistItr_ = playlists_.begin();
    }

    //If this operation takes too much time, we can make it happen after playlist is selected
    //and we are moving to the playing music menu.
    tPlaylist* playlist = *currentPlaylistItr_;
    playlist->LoadPlaylist();
}

inline void tMusicManager::PreviousPlaylist(){
    if(currentPlaylistItr_ == playlists_.end()){
        Log::Error("Current Playlist Iterator invalid");
        return;
    }

    if(currentPlaylistItr_ == playlists_.begin()){
        currentPlaylistItr_ = playlists_.end();
    }

    --currentPlaylistItr_;

    //If this operation takes too much time, we can make it happen after playlist is selected
    //and we are moving to the playing music menu.
    tPlaylist* playlist = *currentPlaylistItr_;
    playlist->LoadPlaylist();
}

inline void tMusicManager::SetVolumeStep(tVolumeValue step) {
    if (step >= 100) {
        step = 100;
    }

    volumeStep_ = step;
}

// we gotta be careful with the dynamic allocation.
// this function should only ever be called on startup.
inline void tMusicManager::BuildPlaylists(){
    playlists_.clear();
    currentPlaylistItr_ = playlists_.end();

    auto folders = fileManager.GetFolders();
    for(auto folder : folders){
        tPlaylist* p = new tPlaylist(folder);
        playlists_.push_back(p);
    }

    currentPlaylistItr_ = playlists_.begin();
}

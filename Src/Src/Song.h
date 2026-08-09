#pragma once

#include <vector>

#include "../HAL/Conversion.h"
#include "../HAL/HAL.h"
#include "FileManager.h"
#include "Log.h"

class tSong {
   public:
    tSong(tFile* file);

    tFile* File();
    const char* SongName();

   private:
    tFile* file_{nullptr};
};

inline tSong::tSong(tFile* file) : file_(file) {}
inline tFile* tSong::File() { return file_; }
inline const char* tSong::SongName() { return file_->GetFileName(); }

class tPlaylist {
   public:
    tPlaylist();

    const char* PlaylistName();

    void LoadPlaylist();
    void UnloadPlaylist();

   private:
    std::vector<tSong*> songs_;
    tFolder* folder_;

    bool playlistLoaded_{false};
};

inline tPlaylist::tPlaylist() {}
inline const char* tPlaylist::PlaylistName() {
    return folder_->GetFolderName();
}

inline void tPlaylist::LoadPlaylist() {
    bool success = false;
    // based on all the mp3 files under the folder_.FullPath
    // build up a vecotr of songs.
    songs_.clear();
    std::vector<tFile*> files = tFileManager::GetFiles(folder_->GetFullPath());
    for (tFile* file : files) {
        songs_.push_back(new tSong(file));
    }
    playlistLoaded_ = true;
}

inline void tPlaylist::UnloadPlaylist() {
    songs_.clear();
    playlistLoaded_ = false;
}

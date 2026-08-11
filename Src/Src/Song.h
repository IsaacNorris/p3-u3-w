#pragma once

#include <vector>

#include "../HAL/Conversion.h"
#include "../HAL/HAL.h"
#include "FileManager.h"
#include "Log.h"

inline constexpr char AudioFileExtension[] = "mp3";

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
    tPlaylist(tFolder* folder);

    tSong* CurrentSong();

    const char* PlaylistName();

    void LoadPlaylist();
    void UnloadPlaylist();

    void NextSong();
    void PreviousSong();

   private:
    std::vector<tSong*> songs_;
    std::vector<tSong*>::iterator currentSongItr_{songs_.end()};

    tFolder* folder_{};

    bool playlistLoaded_{false};
};

inline tPlaylist::tPlaylist(tFolder* folder) : folder_(folder) {}

inline tSong* tPlaylist::CurrentSong() {
    if (currentSongItr_ == songs_.end()) {
        return nullptr;
    }
    return *currentSongItr_;
}

inline const char* tPlaylist::PlaylistName() {
    return folder_->GetFolderName();
}

inline void tPlaylist::NextSong() {
    if (currentSongItr_ == songs_.end()) {
        Log::Error("Current Song Iterator invalid");
        return;
    }

    ++currentSongItr_;

    if (currentSongItr_ == songs_.end()) {
        currentSongItr_ = songs_.begin();
    }
}

inline void tPlaylist::PreviousSong() {
    if (currentSongItr_ == songs_.end()) {
        Log::Error("Current Song Iterator invalid");
        return;
    }

    if (currentSongItr_ == songs_.begin()) {
        currentSongItr_ = songs_.end();
    }

    --currentSongItr_;
}

inline void tPlaylist::LoadPlaylist() {
    bool success = false;
    // based on all the mp3 files under the folder_.FullPath
    // build up a vector of songs.

    songs_.clear();
    currentSongItr_ = songs_.end();
    std::vector<tFile*> files =
        fileManager.GetFilesInFolder(folder_->GetFullPath(), AudioFileExtension);

    for (tFile* file : files) {
        songs_.push_back(new tSong(file));
    }

    // TODO: find a better way of tracking success;
    success = true;

    playlistLoaded_ = success;

    currentSongItr_ = songs_.begin();
}

inline void tPlaylist::UnloadPlaylist() {
    songs_.clear();
    currentSongItr_ = songs_.end();
    playlistLoaded_ = false;
}

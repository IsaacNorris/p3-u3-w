#pragma once

#include "../HAL/HAL.h"
#include "../HAL/Conversion.h"
#include "Log.h"
#include "FileManager.h"

#include <vector>

class tSong {
   public:
    tSong();

    tFile* File();
    const char* SongName();

   private:
    tFile* file_;
};

inline tSong::tSong() {}
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

    bool playlistLoaded_ {false};
};

inline tPlaylist::tPlaylist() {}
inline const char* tPlaylist::PlaylistName() { return folder_->GetFolderName(); }


inline void tPlaylist::LoadPlaylist(){
    bool success = false;
    //based on all the mp3 files under the folder_.FullPath
    //build up a vecotr of songs.



    playlistLoaded_ = success;
}

inline void tPlaylist::UnloadPlaylist(){
    //clear the vector of songs.



    playlistLoaded_ = false;
}

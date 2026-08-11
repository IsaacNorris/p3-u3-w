#pragma once

#include <cstddef>
#include <vector>

#include "../HAL/Conversion.h"
#include "../HAL/HAL.h"
#include "AudioFormat.h"
#include "FileManager.h"
#include "Log.h"

// A song is the file that holds it and nothing else. The file is held by value,
// which is what lets a playlist own its songs outright: there is no separately
// allocated tFile behind a pointer for anyone to have to free.
class tSong {
   public:
    explicit tSong(const tFile& file) : file_(file) {}

    const tFile& File() const { return file_; }
    const char* SongName() const { return file_.GetFileName(); }

   private:
    tFile file_;
};

// Owns its songs by value. Positions are held as an index rather than an
// iterator so that a playlist stays copyable and movable, which is what a
// vector of playlists needs when it grows.
class tPlaylist {
   public:
    explicit tPlaylist(const tFolder& folder) : folder_(folder) {}

    // Null when the playlist holds no songs, which is the case for any folder
    // on the card that has nothing playable in it.
    tSong* CurrentSong();

    const char* PlaylistName() const { return folder_.GetFolderName(); }
    size_t SongCount() const { return songs_.size(); }
    bool IsLoaded() const { return playlistLoaded_; }

    void LoadPlaylist();
    void UnloadPlaylist();

    void NextSong();
    void PreviousSong();

   private:
    std::vector<tSong> songs_;
    size_t currentSongIndex_{0};

    tFolder folder_;

    bool playlistLoaded_{false};
};

inline tSong* tPlaylist::CurrentSong() {
    if (currentSongIndex_ >= songs_.size()) {
        return nullptr;
    }
    return &songs_[currentSongIndex_];
}

inline void tPlaylist::NextSong() {
    if (songs_.empty()) {
        Log::Error("Playlist holds no songs");
        return;
    }

    ++currentSongIndex_;
    if (currentSongIndex_ >= songs_.size()) {
        currentSongIndex_ = 0;
    }
}

inline void tPlaylist::PreviousSong() {
    if (songs_.empty()) {
        Log::Error("Playlist holds no songs");
        return;
    }

    if (currentSongIndex_ == 0) {
        currentSongIndex_ = songs_.size();
    }
    --currentSongIndex_;
}

inline void tPlaylist::LoadPlaylist() {
    songs_.clear();
    currentSongIndex_ = 0;

    // Every file in the folder is offered up and AudioFormat decides what is
    // playable, rather than the walk filtering on one extension. That way a
    // format only has to be taught to AudioFormat once for playlists to start
    // listing it.
    fileManager.ForEachFile(
        folder_.GetFullPath(),
        [](const tFile& file, void* context) {
            if (AudioFormat::FromExtension(file.GetExtension()) !=
                eAudioFormat::Unknown) {
                static_cast<std::vector<tSong>*>(context)->emplace_back(file);
            }
            return true;
        },
        &songs_);

    playlistLoaded_ = !songs_.empty();
    if (!playlistLoaded_) {
        Log::Warning("Playlist holds no playable files");
    }
}

inline void tPlaylist::UnloadPlaylist() {
    songs_.clear();
    currentSongIndex_ = 0;
    playlistLoaded_ = false;
}

#pragma once

#include <cstring>
#include <vector>

#include "AudioPlayer.h"
#include "FileManager.h"
#include "Song.h"

class tMusicManager {
   public:
    tMusicManager();

    void Update();

    tPlaylist* CurrentPlaylist();
    tSong* CurrentSong();

    tVolumeValue Volume();
    void IncrementVolume();
    void DecrementVolume();
    void NextSong();
    void PreviousSong();
    void PlaySong();
    void PausePlaySong();
    void NextPlaylist();
    void PreviousPlaylist();

    void SetVolumeStep(tVolumeValue step);

    bool IsPlaying() const { return isPlaying_; }
    uint32_t SongPositionMs() const { return audioPlayer_.PositionMs(); }
    uint32_t SongDurationMs() const { return audioPlayer_.DurationMs(); }

    // We gotta be careful with the dynamic allocation, so this should only ever
    // be called on startup, once the card is mounted.
    void BuildPlaylists();

   private:
    tAudioPlayer audioPlayer_;

    // Playlists are owned by value, and the position in them is an index rather
    // than an iterator, so growing the vector cannot leave anything dangling.
    std::vector<tPlaylist> playlists_;
    size_t currentPlaylistIndex_{0};

    tVolume volume_;
    tVolumeValue volumeStep_{5};

    // What the user last asked for, which is not the same as what the player is
    // doing: a song that has played out leaves this set so the next one starts
    // on its own.
    bool isPlaying_{false};

    bool StartCurrentSong();
    void PushVolume();
    bool IsCurrentSongLoaded();
};

inline tMusicManager::tMusicManager() {}

inline void tMusicManager::Update() {
    audioPlayer_.Update();

    // A song playing itself out is the only thing that moves the playlist on
    // without the user asking.
    if (isPlaying_ && audioPlayer_.IsFinished()) {
        NextSong();

        // Still finished means nothing started, so stop asking every tick.
        if (audioPlayer_.IsFinished()) {
            isPlaying_ = false;
        }
    }
}

inline tPlaylist* tMusicManager::CurrentPlaylist() {
    if (currentPlaylistIndex_ >= playlists_.size()) {
        return nullptr;
    }
    return &playlists_[currentPlaylistIndex_];
}

inline tSong* tMusicManager::CurrentSong() {
    tPlaylist* playlist = CurrentPlaylist();
    if (playlist == nullptr) {
        return nullptr;
    }
    return playlist->CurrentSong();
}

inline tVolumeValue tMusicManager::Volume() { return volume_.Volume(); }

inline void tMusicManager::PushVolume() {
    audioPlayer_.SetVolume(volume_.Volume());
}

inline void tMusicManager::IncrementVolume() {
    volume_ += volumeStep_;
    PushVolume();
}

inline void tMusicManager::DecrementVolume() {
    volume_ -= volumeStep_;
    PushVolume();
}

inline bool tMusicManager::StartCurrentSong() {
    tSong* song = CurrentSong();
    if (song == nullptr) {
        Log::Warning("No song to play");
        isPlaying_ = false;
        return false;
    }

    PushVolume();
    if (!audioPlayer_.Play(song->File())) {
        // Left stopped rather than skipped on, so a folder of unplayable files
        // cannot walk the whole playlist in one tick.
        isPlaying_ = false;
        return false;
    }

    isPlaying_ = true;
    return true;
}

inline void tMusicManager::NextSong() {
    tPlaylist* playlist = CurrentPlaylist();
    if (playlist == nullptr) {
        Log::Error("Current Playlist Iterator invalid");
        return;
    }

    playlist->NextSong();

    if (isPlaying_) {
        StartCurrentSong();
    }
}

inline void tMusicManager::PreviousSong() {
    tPlaylist* playlist = CurrentPlaylist();
    if (playlist == nullptr) {
        Log::Error("Current Playlist Iterator invalid");
        return;
    }

    playlist->PreviousSong();

    if (isPlaying_) {
        StartCurrentSong();
    }
}

inline bool tMusicManager::IsCurrentSongLoaded() {
    tSong* song = CurrentSong();
    if (song == nullptr) {
        return false;
    }
    return std::strcmp(audioPlayer_.Source().GetFullPath(),
                       song->File().GetFullPath()) == 0;
}

// Starting playback outright, as opposed to PausePlaySong toggling it. Picking
// a playlist and then pressing play should get you the song the playlist is
// sitting on, not a pause.
inline void tMusicManager::PlaySong() {
    if (audioPlayer_.IsPaused() && IsCurrentSongLoaded()) {
        audioPlayer_.Resume();
        isPlaying_ = true;
        return;
    }

    if (audioPlayer_.IsPlaying() && IsCurrentSongLoaded()) {
        return;
    }

    StartCurrentSong();
}

inline void tMusicManager::PausePlaySong() {
    if (audioPlayer_.IsPaused()) {
        audioPlayer_.Resume();
        isPlaying_ = true;
        return;
    }

    if (audioPlayer_.IsActive()) {
        audioPlayer_.Pause();
        isPlaying_ = false;
        return;
    }

    // Nothing loaded, or the last song ran out with playback switched off.
    StartCurrentSong();
}

inline void tMusicManager::NextPlaylist() {
    if (playlists_.empty()) {
        Log::Error("No playlists to move through");
        return;
    }

    ++currentPlaylistIndex_;
    if (currentPlaylistIndex_ >= playlists_.size()) {
        currentPlaylistIndex_ = 0;
    }

    // If this operation takes too much time, we can make it happen after
    // playlist is selected and we are moving to the playing music menu.
    playlists_[currentPlaylistIndex_].LoadPlaylist();
}

inline void tMusicManager::PreviousPlaylist() {
    if (playlists_.empty()) {
        Log::Error("No playlists to move through");
        return;
    }

    if (currentPlaylistIndex_ == 0) {
        currentPlaylistIndex_ = playlists_.size();
    }
    --currentPlaylistIndex_;

    // If this operation takes too much time, we can make it happen after
    // playlist is selected and we are moving to the playing music menu.
    playlists_[currentPlaylistIndex_].LoadPlaylist();
}

inline void tMusicManager::SetVolumeStep(tVolumeValue step) {
    if (step >= 100) {
        step = 100;
    }

    volumeStep_ = step;
}

inline void tMusicManager::BuildPlaylists() {
    playlists_.clear();
    currentPlaylistIndex_ = 0;

    fileManager.ForEachFolder(
        "/",
        [](const tFolder& folder, void* context) {
            static_cast<std::vector<tPlaylist>*>(context)->emplace_back(folder);
            return true;
        },
        &playlists_);

    if (playlists_.empty()) {
        Log::Warning("No playlists found on the card");
        return;
    }

    playlists_[currentPlaylistIndex_].LoadPlaylist();
}

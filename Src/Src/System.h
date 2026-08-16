#pragma once

#include "../HAL/Conversion.h"
#include "../HAL/HAL.h"
#include "Button.h"
#include "Display.h"
#include "FileManager.h"
#include "Log.h"
#include "MusicManager.h"

class tSystem {
   public:
    tSystem();

    void Update();

   private:
    enum class eSystemState {
        Initialization,
        MainPage,
        PlayingMusic,
        PlayListSelection,
        Settings,
        Pedometer,
        Sleep,
        Error,
    };

    tButton buttonPlay_{HAL::eDigitalInput::ButtonPlay};
    tButton buttonNext_{HAL::eDigitalInput::ButtonNext};
    tButton buttonPrevious_{HAL::eDigitalInput::ButtonPrevious};
    tButton buttonMenu_{HAL::eDigitalInput::ButtonMenu};
    tButton buttonVolumeUp_{HAL::eDigitalInput::ButtonVolumeUp};
    tButton buttonVolumeDown_{HAL::eDigitalInput::ButtonVolumeDown};

    eSystemState systemState_{eSystemState::Initialization};
    tBatteryPercentage batteryPercentage_{};
    tMusicManager musicManager_{};
    tDisplay display_;
    tTimeMs lastMountRetryMs_{0};

    void SetState(eSystemState state);

    void UpdateButtons();
    void UpdateBattery();
    void UpdateSystem();

    void SystemStateInitialization();
    void SystemStateMainPage();
    void SystemStatePlayingMusic();
    void SystemStatePlayListSelection();
    void SystemStateSettings();
    void SystemStatePedometer();
    void SystemStateSleep();
    void SystemStateError();
};

inline tSystem::tSystem() {}

inline void tSystem::Update() {
    UpdateButtons();
    UpdateBattery();

    UpdateSystem();

    musicManager_.Update();
}

inline void tSystem::UpdateButtons() {
    buttonPlay_.Update();
    buttonNext_.Update();
    buttonPrevious_.Update();
    buttonMenu_.Update();
    buttonVolumeUp_.Update();
    buttonVolumeDown_.Update();
}

inline void tSystem::UpdateBattery() {
    batteryPercentage_ = Conversion::VoltageToBatteryPercentage(
        HAL::ReadAnalogInput(HAL::eAnalogInput::BatteryPercentage));
}

inline void tSystem::SetState(eSystemState state) {
    // log to see when we change state, could probably replace with display when
    // working.
    switch (state) {
        case eSystemState::Initialization:
            Log::Raw("Init");
            break;
        case eSystemState::MainPage:
            Log::Raw("Main");
            break;
        case eSystemState::PlayingMusic:
            Log::Raw("Music");
            break;
        case eSystemState::PlayListSelection:
            Log::Raw("playlist");
            break;
        case eSystemState::Settings:
            Log::Raw("Settings");
            break;
        case eSystemState::Pedometer:
            Log::Raw("pedometer");
            break;
        case eSystemState::Sleep:
            Log::Raw("sleep");
            break;
        case eSystemState::Error:
            Log::Raw("Error");
            break;
    }

    systemState_ = state;
}

// menu button is back button
// playbutton is select button
// Next Previous are selection(next and previous)
// volume buttons should be always volume control.

// Battery, Volume should always be on display.
// maybe time <- if its accurate.

inline void tSystem::UpdateSystem() {
    switch (systemState_) {
        case eSystemState::Initialization:
            SystemStateInitialization();
            break;
        case eSystemState::MainPage:
            SystemStateMainPage();
            break;
        case eSystemState::PlayingMusic:
            SystemStatePlayingMusic();
            break;
        case eSystemState::PlayListSelection:
            SystemStatePlayListSelection();
            break;
        case eSystemState::Settings:
            SystemStateSettings();
            break;
        case eSystemState::Pedometer:
            SystemStatePedometer();
            break;
        case eSystemState::Sleep:
            SystemStateSleep();
            break;
        case eSystemState::Error:
            SystemStateError();
            break;
    }
}

inline void tSystem::SystemStateInitialization() {
    display_.DisplayInitialisationScreen();
    Log::Custom("SystemState", "Init");

    if (!fileManager.Mount()) {
        Log::Error("Failed to mount the SD card");
        SetState(eSystemState::Error);
        return;
    }

    musicManager_.BuildPlaylists();

    SetState(eSystemState::MainPage);
}

inline void tSystem::SystemStateMainPage() {
    display_.DisplayMainMenuScreen();
    Log::Custom("SystemState", "MainPage");

    if (buttonPlay_.IsReleased()) {
        SetState(eSystemState::PlayListSelection);
    }
}

inline void tSystem::SystemStatePlayingMusic() {
    tSong* song = musicManager_.CurrentSong();
    display_.DisplayMusicPlayingScreen(song == nullptr ? "" : song->SongName(),
                                       batteryPercentage_,
                                       musicManager_.Volume());
    Log::Custom("SystemState", "PlayingMusic");

    if (buttonMenu_.IsReleased()) {
        SetState(eSystemState::PlayListSelection);
    }
    if (buttonPlay_.IsReleased()) {
        musicManager_.PausePlaySong();
    }
    if (buttonVolumeUp_.IsReleased()) {
        musicManager_.IncrementVolume();
    }
    if (buttonVolumeDown_.IsReleased()) {
        musicManager_.DecrementVolume();
    }
    if (buttonNext_.IsReleased()) {
        musicManager_.NextSong();
    }
    if (buttonPrevious_.IsReleased()) {
        musicManager_.PreviousSong();
    }
}

inline void tSystem::SystemStatePlayListSelection() {
    tPlaylist* playlist = musicManager_.CurrentPlaylist();
    display_.DisplayPlaylistScreen(
        playlist == nullptr ? "" : playlist->PlaylistName(), batteryPercentage_,
        musicManager_.Volume());
    Log::Custom("SystemState", "PlaylistSelection");

    if (buttonMenu_.IsReleased()) {
        SetState(eSystemState::MainPage);
    }
    if (buttonPlay_.IsReleased()) {
        // Selecting a playlist is what starts it playing.
        musicManager_.PlaySong();
        SetState(eSystemState::PlayingMusic);
    }
    if (buttonVolumeUp_.IsReleased()) {
        musicManager_.IncrementVolume();
    }
    if (buttonVolumeDown_.IsReleased()) {
        musicManager_.DecrementVolume();
    }
    if (buttonNext_.IsReleased()) {
        musicManager_.NextPlaylist();
    }
    if (buttonPrevious_.IsReleased()) {
        musicManager_.PreviousPlaylist();
    }
}

inline void tSystem::SystemStateSettings() {
    display_.DisplaySettingsScreen();
    Log::Custom("SystemState", "Setting");

    if (buttonMenu_.IsReleased()) {
        SetState(eSystemState::MainPage);
    }
}

inline void tSystem::SystemStatePedometer() {
    // display_.DisplayPedometerScreen(pedometer_.Steps());
    Log::Custom("SystemState", "Pedometer");
}

inline void tSystem::SystemStateSleep() {
    display_.DisplaySleepScreen();
    Log::Custom("SystemState", "Sleep");
}

inline void tSystem::SystemStateError() {
    display_.DisplayErrorScreen();

    // A card that was not ready on the first tick often is a second later, and
    // sitting in Error forever for that is worse than trying again. Kept slow
    // because a failed begin that is retried every couple of seconds never
    // gives the card a chance to see an idle bus.
    const tTimeMs now = HAL::GetCurrentTimeMs();
    if (lastMountRetryMs_ != 0 && (now - lastMountRetryMs_) < 15000) {
        return;
    }
    lastMountRetryMs_ = now;

    if (!fileManager.Mount()) {
        return;
    }

    musicManager_.BuildPlaylists();
    SetState(eSystemState::MainPage);
}

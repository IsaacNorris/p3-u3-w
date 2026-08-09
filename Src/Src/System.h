#pragma once

#include "../HAL/Conversion.h"
#include "../HAL/HAL.h"
#include "Button.h"
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

inline void tSystem::SetState(eSystemState state) { systemState_ = state; }

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
    Log::Custom("SystemState", "Init");

    SetState(eSystemState::MainPage);
}

inline void tSystem::SystemStateMainPage() {
    Log::Custom("SystemState", "MainPage");

    if (buttonPlay_.IsReleased()) {
        SetState(eSystemState::PlayListSelection);
    }
}

inline void tSystem::SystemStatePlayingMusic() {
    Log::Custom("SystemState", "PlayingMusic");

    if (buttonMenu_.IsPressed()) {
        if (buttonPlay_.IsClicked()) {
            SetState(eSystemState::MainPage);
        }
    }
    if (buttonMenu_.IsReleased()) {
        SetState(eSystemState::PlayListSelection);
    }
    if (buttonPlay_.IsReleased()) {
        // audioPlayer_.PlayPause();
    }
    if (buttonVolumeUp_.IsReleased()) {
        musicManager_.DecrementVolume();
    }
    if (buttonVolumeDown_.IsReleased()) {
        musicManager_.IncrementVolume();
    }
    if (buttonNext_.IsReleased()) {
        // audioPlayer_.Next();
    }
    if (buttonPrevious_.IsReleased()) {
        // audioPlayer_.Previous();
    }
}

inline void tSystem::SystemStatePlayListSelection() {
    Log::Custom("SystemState", "PlaylistSelection");

    if (buttonMenu_.IsPressed()) {
        if (buttonPlay_.IsClicked()) {
            SetState(eSystemState::MainPage);
        }
    }
    if (buttonMenu_.IsReleased()) {
        SetState(eSystemState::PlayingMusic);
    }
    if (buttonPlay_.IsReleased()) {
        // audioPlayer_.PlayPause();
    }
    if (buttonVolumeUp_.IsReleased()) {
        // audioPlayer_.DecrementVolume();
    }
    if (buttonVolumeDown_.IsReleased()) {
        // audioPlayer_.IncrementVolume();
    }
    if (buttonNext_.IsReleased()) {
        // audioPlayer_.Next();
    }
    if (buttonPrevious_.IsReleased()) {
        // audioPlayer_.Previous();
    }

    // Dummy code:
    if (buttonMenu_.HeldForMs() >= 1500) {
        SetState(eSystemState::Settings);
    }
}

inline void tSystem::SystemStateSettings() {
    Log::Custom("SystemState", "Setting");

    if (buttonMenu_.IsClicked()) {
        SetState(eSystemState::MainPage);
    }
}

inline void tSystem::SystemStatePedometer() {
    Log::Custom("SystemState", "Pedometer");
}

inline void tSystem::SystemStateSleep() { Log::Custom("SystemState", "Sleep"); }

inline void tSystem::SystemStateError() { Log::Error("SystemState"); }

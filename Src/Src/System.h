#pragma once

#include "AudioPlayer.h"
#include "Button.h"
#include "HAL.h"
#include "Log.h"

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
        Sleep,
        Error,
    };

    tButton buttonPlay_{HAL::eDigitalInput::ButtonPlay};
    tButton buttonSkip_{HAL::eDigitalInput::ButtonSkip};
    tButton buttonMenu_{HAL::eDigitalInput::ButtonMenu};
    tButton buttonVolumeUp_{HAL::eDigitalInput::ButtonVolumeUp};
    tButton buttonVolumeDown_{HAL::eDigitalInput::ButtonVolumeDown};

    tAudioPlayer audioPlayer_;

    eSystemState systemState_{eSystemState::Initialization};

    void UpdateButtons();

    void UpdateSystem();
};

inline tSystem::tSystem() {}

inline void tSystem::UpdateButtons() {
    buttonPlay_.Update();
    buttonSkip_.Update();
    buttonMenu_.Update();
    buttonVolumeUp_.Update();
    buttonVolumeDown_.Update();
}

inline void tSystem::Update() {
    UpdateButtons();

    UpdateSystem();
}

inline void tSystem::UpdateSystem() {
    switch (systemState_) {
        case eSystemState::Initialization:
            systemState_ = eSystemState::MainPage;
            break;
        case eSystemState::PlayingMusic:
            break;
        case eSystemState::PlayListSelection:
            break;
        case eSystemState::Settings:
            break;
        case eSystemState::Sleep:
            break;
        case eSystemState::Error:
            break;
    }
}

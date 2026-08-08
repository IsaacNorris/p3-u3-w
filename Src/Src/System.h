#pragma once

#include "AudioPlayer.h"
#include "Button.h"
#include "../HAL/HAL.h"
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
    void SetState(eSystemState state);

    void UpdateSystem();
};

inline tSystem::tSystem() {}

inline void tSystem::Update() {
    UpdateButtons();
    UpdateSystem();
}

inline void tSystem::UpdateButtons() {
    buttonPlay_.Update();
    buttonSkip_.Update();
    buttonMenu_.Update();
    buttonVolumeUp_.Update();
    buttonVolumeDown_.Update();
}

inline void tSystem::SetState(eSystemState state){
    systemState_ = state;
}

inline void tSystem::UpdateSystem() {
    switch (systemState_) {
        case eSystemState::Initialization:
            Log::Custom("SystemState", "Init");

            SetState(eSystemState::MainPage);
            break;
        case eSystemState::MainPage:
            Log::Custom("SystemState", "MainPage");

            if(buttonPlay_.IsReleased()){
                SetState(eSystemState::PlayListSelection);
            }
            break;
        case eSystemState::PlayingMusic:
            Log::Custom("SystemState", "PlayingMusic");

            if(buttonMenu_.IsPressed()){
                if(buttonPlay_.IsClicked()){
                    SetState(eSystemState::MainPage);
                }
            }
            if(buttonMenu_.IsReleased()){
                SetState(eSystemState::PlayListSelection);
            }
            if(buttonPlay_.IsReleased()){
                // audioPlayer_.PlayPause();
            }
            if(buttonVolumeUp_.IsReleased()){
                // audioPlayer_.DecrementVolume();
            }
            if(buttonVolumeDown_.IsReleased()){
                // audioPlayer_.IncrementVolume();
            }
            if(buttonSkip_.IsReleased()){
                // audioPlayer_.Next();
            }
            break;
        case eSystemState::PlayListSelection:
            Log::Custom("SystemState", "PlaylistSelection");

            if(buttonMenu_.IsPressed()){
                if(buttonPlay_.IsClicked()){
                    SetState(eSystemState::MainPage);
                }
            }
            if(buttonMenu_.IsReleased()){
                SetState(eSystemState::PlayingMusic);
            }
            if(buttonPlay_.IsReleased()){
                // audioPlayer_.PlayPause();
            }
            if(buttonVolumeUp_.IsReleased()){
                // audioPlayer_.DecrementVolume();
            }
            if(buttonVolumeDown_.IsReleased()){
                // audioPlayer_.IncrementVolume();
            }
            if(buttonSkip_.IsReleased()){
                // audioPlayer_.Next();
            }
            break;
        case eSystemState::Settings:
            Log::Custom("SystemState", "Setting");

            break;
        case eSystemState::Sleep:
            Log::Custom("SystemState", "Sleep");

            break;
        case eSystemState::Error:
            Log::Error("SystemState");

            break;
    }
}

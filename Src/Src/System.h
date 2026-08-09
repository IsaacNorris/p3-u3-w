#pragma once

#include "../HAL/HAL.h"
#include "../HAL/Conversion.h"
#include "AudioPlayer.h"
#include "Button.h"
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

    tAudioPlayer audioPlayer_;

    eSystemState systemState_{eSystemState::Initialization};
    tBatteryPercentage batteryPercentage_ {};

    void SetState(eSystemState state);

    void UpdateButtons();
    void UpdateBattery();
    void UpdateSystem();
};

inline tSystem::tSystem() {}

inline void tSystem::Update() {
    UpdateButtons();
    UpdateBattery();
    UpdateSystem();
}

inline void tSystem::UpdateButtons() {
    buttonPlay_.Update();
    buttonNext_.Update();
    buttonPrevious_.Update();
    buttonMenu_.Update();
    buttonVolumeUp_.Update();
    buttonVolumeDown_.Update();
}

inline void tSystem::UpdateBattery(){
    batteryPercentage_ = Conversion::VoltageToBatteryPercentage(HAL::ReadAnalogInput(HAL::eAnalogInput::BatteryPercentage));
}

inline void tSystem::SetState(eSystemState state) { systemState_ = state; }

inline void tSystem::UpdateSystem() {
    switch (systemState_) {
        case eSystemState::Initialization:
            Log::Custom("SystemState", "Init");

            SetState(eSystemState::MainPage);
            break;
        case eSystemState::MainPage:
            Log::Custom("SystemState", "MainPage");

            if (buttonPlay_.IsReleased()) {
                SetState(eSystemState::PlayListSelection);
            }
            break;
        case eSystemState::PlayingMusic:
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
            break;
        case eSystemState::PlayListSelection:
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

            //Dummy code:
            if(buttonMenu_.HeldForMs() >= 1500){
                SetState(eSystemState::Settings);
            }

            break;
        case eSystemState::Settings:
            Log::Custom("SystemState", "Setting");

            if(buttonMenu_.IsClicked()){
                SetState(eSystemState::MainPage);
            }
            break;
        case eSystemState::Pedometer:
            Log::Custom("SystemState", "Pedometer");

            break;
        case eSystemState::Sleep:
            Log::Custom("SystemState", "Sleep");

            break;
        case eSystemState::Error:
            Log::Error("SystemState");

            break;
    }
}

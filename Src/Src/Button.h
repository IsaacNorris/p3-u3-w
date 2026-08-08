#pragma once

#include "../HAL/HAL.h"

class tButton {
   public:
    tButton() = default;
    tButton(HAL::eDigitalInput button, tTimeMs debounceTime_ = 25);

    void Update();
    void SetButton(HAL::eDigitalInput button);

    bool IsPressed() const;
    bool IsReleased() const;
    bool IsClicked() const;

   private:
    bool currentState_{false};
    bool isClicked_{false};
    bool isReleased_{false};
    HAL::eDigitalInput button_;
    tTimeMs debounceTime_;
    tTimeMs lastPressedTime_;
};

inline tButton::tButton(HAL::eDigitalInput button, tTimeMs debounceTime) {
    button_ = button;
    debounceTime_ = debounceTime;
    lastPressedTime_ = HAL::GetCurrentTimeMs();
}

inline void tButton::Update() {
    bool readState = HAL::ReadDigitalInput(button_);
    tTimeMs passedTime = HAL::GetCurrentTimeMs() - lastPressedTime_;

    isClicked_ = false;
    isReleased_ = false;

    if (passedTime > debounceTime_) {
        if (readState != currentState_) {
            lastPressedTime_ = HAL::GetCurrentTimeMs();
            currentState_ = readState;

            if (currentState_) {
                isClicked_ = true;
            } else {
                isReleased_ = true;
            }
        }
    }
}

inline bool tButton::IsPressed() const { return currentState_; }

inline void tButton::SetButton(HAL::eDigitalInput button) { button_ = button; }

inline bool tButton::IsClicked() const { return isClicked_; }

inline bool tButton::IsReleased() const { return isReleased_; }

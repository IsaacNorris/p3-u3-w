#pragma once

#include "HAL.h"

class tElapsed {
   public:
    tElapsed();

    bool IsElapsed(tTimeMs timeToCheck) const;
    bool IsElapsedRestart(tTimeMs timeToCheck) const;
    void Restart();

   private:
    mutable tTimeMs lastTime_ = HAL::CurrentTime();
    mutable bool expired_{};
};

inline tElapsed::tElapsed() {}

inline bool tElapsed::IsElapsed(tTimeMs timeToCheck) const {
    if (!expired_) {
        expired_ =
            HAL::CurrentTime() >= static_cast<tTimeMs>(lastTime_ + timeToCheck);
    }

    return expired_;
}

inline bool tElapsed::IsElapsedRestart(tTimeMs timeToCheck) const {
    tTimeMs currentTime = HAL::GetCurrentTimeMs();

    if (expired_ || (currentTime - lastTime_ >= timeToCheck)) {
        lastTime_ = currentTime;
        expired_ = false;

        return true;
    }

    return false;
}

inline void tElapsed::Restart() { *this = {}; }
#pragma once

#include "../Drivers/AudioDriver.h"
#include "../HAL/HAL.h"
#include "Log.h"

struct tVolume{
   public:
    tVolume& operator+(tVolumeValue value){
        SetVolume(Volume() + value);
        return *this;
    }

    tVolume& operator-(tVolumeValue value){
        tVolumeValue next = Volume();
        if(next < value){
            next = 0;
        }else{
            next -= value;
        }
        SetVolume(next);
        return *this;
    }

    void SetVolume(tVolumeValue volume){
        if(volume > VolumeMax){
            volume_ = VolumeMax;
        }else{
            volume_ = volume;
        }
    }

    tVolumeValue Volume() const{
        return volume_;
    }

   private:
    static constexpr uint8_t VolumeMax = 100;
    tVolumeValue volume_ = {50};
};

class tAudioPlayer {
   public:
    tAudioPlayer();

    void PlayPause();
   private:

};

inline tAudioPlayer::tAudioPlayer() {}


#pragma once

#include "HAL.h"

class Conversion{
   public:
    template<typename T>
    static T Map(const T& minInput, const T& maxInput, const T& minOutput, const T& maxOutput, const T& value){
        const T slope = 1 * (maxOutput - minOutput)/(maxInput - minInput);
        return slope * (value - minInput) + minOutput;
    }

    static tBatteryPercentage VoltageToBatteryPercentage(tVoltage value){
        static constexpr tVoltage MinValueIn = 0.0;
        static constexpr tVoltage MaxValueIn = 3.3;
        static constexpr tVoltage MinValueOut = 0.0;
        static constexpr tVoltage MaxValueOut = 100.0;

        return static_cast<tBatteryPercentage>(Conversion::Map<tVoltage>(MinValueIn, MaxValueIn, MinValueOut, MaxValueOut, value));
    };
};

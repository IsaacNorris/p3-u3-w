#pragma once

#include "../Drivers/IMUDriver.h"
#include "../HAL/Conversion.h"
#include "../HAL/HAL.h"

class tPedometer {
   public:
    tPedometer();

   private:
    tIMUDriver imuDriver_;
};

inline tPedometer::tPedometer() {}
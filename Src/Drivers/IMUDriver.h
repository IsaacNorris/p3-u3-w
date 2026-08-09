#pragma once

class tIMUDriver {
   public:
    tIMUDriver();

    void Open();
    void Close();
    void Write();
    void Read();
    void IOCTL();

   private:
};

inline tIMUDriver::tIMUDriver() {}

inline void tIMUDriver::Open() {}
inline void tIMUDriver::Close() {}
inline void tIMUDriver::Write() {}
inline void tIMUDriver::Read() {}
inline void tIMUDriver::IOCTL() {}
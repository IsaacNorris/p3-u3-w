#pragma once

class tSDCardDriver {
   public:
    tSDCardDriver();

    void Open();
    void Close();
    void Read();
    void Write();
    void IOCTL();

   private:
};

inline tSDCardDriver::tSDCardDriver() {}

inline void tSDCardDriver::Open() {}

inline void tSDCardDriver::Close() {}

inline void tSDCardDriver::Read() {}

inline void tSDCardDriver::Write() {}
inline void tSDCardDriver::IOCTL() {}

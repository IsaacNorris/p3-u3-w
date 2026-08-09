#pragma once

class tAudioDriver{
public:
    tAudioDriver();

    void Open();
    void Close();
    void Read();
    void Write();
    void IOCTL();
private:

};

inline tAudioDriver::tAudioDriver() {}

inline void tAudioDriver::Open() {}

inline void tAudioDriver::Close() {}

inline void tAudioDriver::Read() {}

inline void tAudioDriver::Write() {}

inline void tAudioDriver::IOCTL() {}

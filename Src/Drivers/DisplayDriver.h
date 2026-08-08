#pragma once

class tDisplayDriver {
   public:
    tDisplayDriver();

    void Open();
    void Close();
    void Write();
    void Read();
    void IOCTL();

   private:
};

inline tDisplayDriver::tDisplayDriver() {}

inline void tDisplayDriver::Open() {}

inline void tDisplayDriver::Close() {}

inline void tDisplayDriver::Write() {}

inline void tDisplayDriver::Read() {}
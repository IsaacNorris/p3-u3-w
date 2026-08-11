#pragma once

#include <cstddef>
#include <cstdint>

// Index into the platform's fixed pool of open files. The pool lives in the
// platform layer (FatFs FIL, FS File, ...) so nothing here has to know how big
// a native handle is, and no dynamic allocation is ever needed.
using tFileHandle = int8_t;
inline constexpr tFileHandle InvalidFileHandle = -1;

// One handle for the audio stream, one for everything else.
inline constexpr uint8_t MaxOpenFiles = 2;

struct tDirectoryEntry {
    // Entry name only, not a path. Valid for the duration of the callback.
    const char* name{nullptr};
    uint32_t size{0};
    bool isDirectory{false};
};

// Return false to stop the walk early.
using tDirectoryCallback = bool (*)(const tDirectoryEntry& entry,
                                    void* context);

// Platform boundary for the SD card. Everything below is declared here and
// implemented once per target, the same way HAL is. The volume level calls are
// static because there is only ever one card, while an instance owns at most
// one open file so streaming audio and touching a text file cannot fight over
// the same handle.
class tSDCardDriver {
   public:
    enum class eOpenMode : uint8_t {
        Read,
        Write,   // truncates, creates when missing
        Append,  // creates when missing
    };

    tSDCardDriver() = default;
    ~tSDCardDriver();

    tSDCardDriver(const tSDCardDriver&) = delete;
    tSDCardDriver& operator=(const tSDCardDriver&) = delete;

    static bool Mount();
    static void Unmount();
    static bool IsMounted();

    static bool Exists(const char* path);
    static bool MakeDirectory(const char* path);
    static bool RemoveDirectory(const char* path);
    static bool Remove(const char* path);

    // Walks one directory without allocating. Entries come back in whatever
    // order the file system gives them, and "." / ".." may be among them, so
    // callers must not rely on either.
    static bool List(const char* path, tDirectoryCallback callback,
                     void* context);

    bool Open(const char* path, eOpenMode mode);
    void Close();
    bool IsOpen() const { return handle_ != InvalidFileHandle; }

    // Both return the number of bytes actually transferred, which is short of
    // size at end of file or on error.
    size_t Read(void* buffer, size_t size);
    size_t Write(const void* buffer, size_t size);

    bool Seek(uint32_t position);
    uint32_t Position() const;
    uint32_t Size() const;
    bool Flush();

   private:
    tFileHandle handle_{InvalidFileHandle};
};

inline tSDCardDriver::~tSDCardDriver() { Close(); }

#include "../../../Src/Drivers/SDCardDriver.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <string>
#include <system_error>

#include "../../../Src/Src/Log.h"

// Simulator backing for the SD card. The card is a folder on the host and the
// open file pool mirrors what the targets will do with FatFs FIL / FS File
// objects, so the application sees identical handle exhaustion behaviour here.

namespace {

std::array<std::FILE*, MaxOpenFiles> openFiles{};
bool mounted = false;

// The .pro pins this to the folder beside the project so that test content
// survives switching between Debug and Release or wiping the build directory.
// Builds that do not set it keep the card in the working directory.
#ifndef SD_CARD_ROOT
#define SD_CARD_ROOT "sdcard"
#endif

std::filesystem::path ResolveSdCardRoot() {
    std::error_code error;
    const std::filesystem::path root(SD_CARD_ROOT);
    const std::filesystem::path absoluteRoot =
        std::filesystem::absolute(root, error);
    return error ? root : absoluteRoot;
}

const std::filesystem::path& SdCardRoot() {
    static const std::filesystem::path root = ResolveSdCardRoot();
    return root;
}

std::filesystem::path HostPath(const char* path) {
    std::filesystem::path host = SdCardRoot();
    if (path == nullptr) {
        return host;
    }

    while (*path == '/') {
        ++path;
    }
    if (*path != '\0') {
        host /= std::filesystem::path(path);
    }
    return host;
}

bool IsValidHandle(tFileHandle handle) {
    return handle >= 0 && handle < static_cast<tFileHandle>(MaxOpenFiles) &&
           openFiles[handle] != nullptr;
}

}  // namespace

bool tSDCardDriver::Mount() {
    std::error_code error;
    std::filesystem::create_directories(SdCardRoot(), error);
    mounted = std::filesystem::is_directory(SdCardRoot(), error);

    const std::string message =
        (mounted ? "SD card mounted at " : "Failed to mount SD card at ") +
        SdCardRoot().string();
    if (mounted) {
        Log::Info(message.c_str());
    } else {
        Log::Error(message.c_str());
    }

    return mounted;
}

void tSDCardDriver::Unmount() {
    for (std::FILE*& file : openFiles) {
        if (file != nullptr) {
            std::fclose(file);
            file = nullptr;
        }
    }
    mounted = false;
}

bool tSDCardDriver::IsMounted() { return mounted; }

bool tSDCardDriver::Exists(const char* path) {
    if (!mounted) {
        return false;
    }
    std::error_code error;
    return std::filesystem::exists(HostPath(path), error);
}

bool tSDCardDriver::MakeDirectory(const char* path) {
    if (!mounted) {
        return false;
    }
    std::error_code error;
    return std::filesystem::create_directory(HostPath(path), error);
}

bool tSDCardDriver::RemoveDirectory(const char* path) {
    if (!mounted) {
        return false;
    }
    std::error_code error;
    return std::filesystem::remove(HostPath(path), error);
}

bool tSDCardDriver::Remove(const char* path) {
    if (!mounted) {
        return false;
    }
    std::error_code error;
    return std::filesystem::remove(HostPath(path), error);
}

bool tSDCardDriver::List(const char* path, tDirectoryCallback callback,
                         void* context) {
    if (!mounted || callback == nullptr) {
        return false;
    }

    std::error_code error;
    std::filesystem::directory_iterator iterator(HostPath(path), error);
    if (error) {
        return false;
    }

    const std::filesystem::directory_iterator end;
    for (; iterator != end; iterator.increment(error)) {
        if (error) {
            return false;
        }

        const std::string name = iterator->path().filename().string();
        tDirectoryEntry entry;
        entry.name = name.c_str();
        entry.isDirectory = iterator->is_directory(error);
        if (!entry.isDirectory) {
            const std::uintmax_t size = iterator->file_size(error);
            entry.size = error ? 0 : static_cast<uint32_t>(size);
            error.clear();
        }

        if (!callback(entry, context)) {
            break;
        }
    }

    return true;
}

bool tSDCardDriver::Open(const char* path, eOpenMode mode) {
    Close();

    if (!mounted || path == nullptr) {
        return false;
    }

    tFileHandle handle = InvalidFileHandle;
    for (uint8_t index = 0; index < MaxOpenFiles; ++index) {
        if (openFiles[index] == nullptr) {
            handle = static_cast<tFileHandle>(index);
            break;
        }
    }
    if (handle == InvalidFileHandle) {
        return false;
    }

    const char* hostMode = "rb";
    switch (mode) {
        case eOpenMode::Read:
            hostMode = "rb";
            break;
        case eOpenMode::Write:
            hostMode = "wb";
            break;
        case eOpenMode::Append:
            hostMode = "ab";
            break;
    }

    const std::string hostPath = HostPath(path).string();
    std::FILE* file = std::fopen(hostPath.c_str(), hostMode);
    if (file == nullptr) {
        return false;
    }

    openFiles[handle] = file;
    handle_ = handle;
    return true;
}

void tSDCardDriver::Close() {
    if (!IsValidHandle(handle_)) {
        handle_ = InvalidFileHandle;
        return;
    }

    std::fclose(openFiles[handle_]);
    openFiles[handle_] = nullptr;
    handle_ = InvalidFileHandle;
}

size_t tSDCardDriver::Read(void* buffer, size_t size) {
    if (!IsValidHandle(handle_) || buffer == nullptr) {
        return 0;
    }
    return std::fread(buffer, 1, size, openFiles[handle_]);
}

size_t tSDCardDriver::Write(const void* buffer, size_t size) {
    if (!IsValidHandle(handle_) || buffer == nullptr) {
        return 0;
    }
    return std::fwrite(buffer, 1, size, openFiles[handle_]);
}

bool tSDCardDriver::Seek(uint32_t position) {
    if (!IsValidHandle(handle_)) {
        return false;
    }
    return std::fseek(openFiles[handle_], static_cast<long>(position),
                      SEEK_SET) == 0;
}

uint32_t tSDCardDriver::Position() const {
    if (!IsValidHandle(handle_)) {
        return 0;
    }
    const long position = std::ftell(openFiles[handle_]);
    return position < 0 ? 0 : static_cast<uint32_t>(position);
}

uint32_t tSDCardDriver::Size() const {
    if (!IsValidHandle(handle_)) {
        return 0;
    }

    std::FILE* file = openFiles[handle_];
    const long position = std::ftell(file);
    if (std::fseek(file, 0, SEEK_END) != 0) {
        return 0;
    }
    const long size = std::ftell(file);
    std::fseek(file, position, SEEK_SET);

    return size < 0 ? 0 : static_cast<uint32_t>(size);
}

bool tSDCardDriver::Flush() {
    if (!IsValidHandle(handle_)) {
        return false;
    }
    return std::fflush(openFiles[handle_]) == 0;
}

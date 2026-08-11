#pragma once

#include <cstddef>
#include <vector>

#include "../Drivers/SDCardDriver.h"
#include "../HAL/HAL.h"
#include "File.h"
#include "Log.h"

class FileManager {
   public:
    // Return false from a visitor to stop the walk early.
    using tFileVisitor = bool (*)(const tFile& file, void* context);
    using tFolderVisitor = bool (*)(const tFolder& folder, void* context);

    FileManager();

    bool Mount();
    void Unmount();
    bool IsMounted() const;

    // Allocation free listing. extensionFilter is optional and case
    // insensitive, pass "mp3" to only visit audio files.
    bool ForEachFile(const char* folderPath, tFileVisitor visitor,
                     void* context, const char* extensionFilter = nullptr);
    bool ForEachFolder(const char* folderPath, tFolderVisitor visitor,
                       void* context);

    // Convenience wrappers over the walks above. Entries come back by value, so
    // there is nothing to free, but the vector itself still allocates. Prefer
    // ForEach* on any path that runs more than once.
    std::vector<tFile> GetFilesInFolder(const char* folderPath,
                                        const char* extensionFilter = nullptr);
    std::vector<tFolder> GetFoldersInFolder(const char* folderPath);

    bool Exists(const char* path);
    bool CreateDir(const char* folderPath);
    bool RemoveDir(const char* folderPath);
    bool RemoveFile(const char* filePath);

    // Reads at most bufferSize - 1 bytes and null terminates, so the buffer is
    // usable as a string. Returns the number of bytes read. Audio is streamed
    // through a tSDCardDriver of its own rather than through here.
    size_t ReadFile(const char* filePath, char* buffer, size_t bufferSize);
    bool WriteFile(const char* filePath, const char* data, size_t size);
    bool AppendFile(const char* filePath, const char* data, size_t size);

   private:
    tSDCardDriver sdCardDriver_;

    bool WriteInternal(const char* filePath, const char* data, size_t size,
                       tSDCardDriver::eOpenMode mode);

    static bool IsDotEntry(const char* name);
};

// global
inline FileManager fileManager;

inline FileManager::FileManager() {}

inline bool FileManager::Mount() { return tSDCardDriver::Mount(); }

inline void FileManager::Unmount() { tSDCardDriver::Unmount(); }

inline bool FileManager::IsMounted() const { return tSDCardDriver::IsMounted(); }

inline bool FileManager::IsDotEntry(const char* name) {
    return name == nullptr || name[0] == '\0' ||
           (name[0] == '.' &&
            (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')));
}

inline bool FileManager::ForEachFile(const char* folderPath,
                                     tFileVisitor visitor, void* context,
                                     const char* extensionFilter) {
    if (visitor == nullptr) {
        return false;
    }

    struct tWalk {
        const char* folderPath;
        const char* extensionFilter;
        tFileVisitor visitor;
        void* context;
    };
    tWalk walk{folderPath, extensionFilter, visitor, context};

    const bool listed = tSDCardDriver::List(
        folderPath,
        [](const tDirectoryEntry& entry, void* context) {
            const tWalk& walk = *static_cast<const tWalk*>(context);
            if (entry.isDirectory || IsDotEntry(entry.name)) {
                return true;
            }

            const tFile file(walk.folderPath, entry.name);
            if (!file.IsValid()) {
                return true;
            }
            if (walk.extensionFilter != nullptr &&
                !file.HasExtension(walk.extensionFilter)) {
                return true;
            }

            return walk.visitor(file, walk.context);
        },
        &walk);

    if (!listed) {
        Log::Error("Failed to open directory");
    }
    return listed;
}

inline bool FileManager::ForEachFolder(const char* folderPath,
                                       tFolderVisitor visitor, void* context) {
    if (visitor == nullptr) {
        return false;
    }

    struct tWalk {
        const char* parentPath;
        tFolderVisitor visitor;
        void* context;
    };
    tWalk walk{folderPath, visitor, context};

    const bool listed = tSDCardDriver::List(
        folderPath,
        [](const tDirectoryEntry& entry, void* context) {
            const tWalk& walk = *static_cast<const tWalk*>(context);
            if (!entry.isDirectory || IsDotEntry(entry.name)) {
                return true;
            }

            const tFolder folder(walk.parentPath, entry.name);
            if (!folder.IsValid()) {
                return true;
            }

            return walk.visitor(folder, walk.context);
        },
        &walk);

    if (!listed) {
        Log::Error("Failed to open directory");
    }
    return listed;
}

inline std::vector<tFile> FileManager::GetFilesInFolder(
    const char* folderPath, const char* extensionFilter) {
    std::vector<tFile> files;

    ForEachFile(
        folderPath,
        [](const tFile& file, void* context) {
            static_cast<std::vector<tFile>*>(context)->push_back(file);
            return true;
        },
        &files, extensionFilter);

    return files;
}

inline std::vector<tFolder> FileManager::GetFoldersInFolder(
    const char* folderPath) {
    std::vector<tFolder> folders;

    ForEachFolder(
        folderPath,
        [](const tFolder& folder, void* context) {
            static_cast<std::vector<tFolder>*>(context)->push_back(folder);
            return true;
        },
        &folders);

    return folders;
}

inline bool FileManager::Exists(const char* path) {
    return tSDCardDriver::Exists(path);
}

inline bool FileManager::CreateDir(const char* folderPath) {
    return tSDCardDriver::MakeDirectory(folderPath);
}

inline bool FileManager::RemoveDir(const char* folderPath) {
    return tSDCardDriver::RemoveDirectory(folderPath);
}

inline bool FileManager::RemoveFile(const char* filePath) {
    return tSDCardDriver::Remove(filePath);
}

inline size_t FileManager::ReadFile(const char* filePath, char* buffer,
                                    size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) {
        return 0;
    }
    buffer[0] = '\0';

    if (!sdCardDriver_.Open(filePath, tSDCardDriver::eOpenMode::Read)) {
        Log::Error("Failed to open file for reading");
        return 0;
    }

    const size_t read = sdCardDriver_.Read(buffer, bufferSize - 1);
    buffer[read] = '\0';
    sdCardDriver_.Close();

    return read;
}

inline bool FileManager::WriteInternal(const char* filePath, const char* data,
                                       size_t size,
                                       tSDCardDriver::eOpenMode mode) {
    if (data == nullptr) {
        return false;
    }

    if (!sdCardDriver_.Open(filePath, mode)) {
        Log::Error("Failed to open file for writing");
        return false;
    }

    const bool written = sdCardDriver_.Write(data, size) == size;
    if (!written) {
        Log::Error("Failed to write the whole file");
    }
    sdCardDriver_.Close();

    return written;
}

inline bool FileManager::WriteFile(const char* filePath, const char* data,
                                   size_t size) {
    return WriteInternal(filePath, data, size,
                         tSDCardDriver::eOpenMode::Write);
}

inline bool FileManager::AppendFile(const char* filePath, const char* data,
                                    size_t size) {
    return WriteInternal(filePath, data, size,
                         tSDCardDriver::eOpenMode::Append);
}

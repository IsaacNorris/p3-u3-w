#pragma once

#include <array>
#include <vector>

#include "../Drivers/SDCardDriver.h"
#include "../HAL/HAL.h"
#include "Log.h"

static constexpr int MaxFilePathLength = 64;

class tFile {
   public:
    tFile(std::array<char, MaxFilePathLength> filePath);

    const char* GetFileName() const;
    const char* GetFullPath() const;

   private:
    std::array<char, MaxFilePathLength> filePath_{};
};

tFile::tFile(std::array<char, MaxFilePathLength> filePath)
    : filePath_(filePath) {}

const char* tFile::GetFileName() const {
    //TODO: add in parsing to get the name.
    return filePath_.data();
}
const char* tFile::GetFullPath() const {
    return filePath_.data();
}

class tFolder {
   public:
    tFolder(std::array<char, MaxFilePathLength> folderPath);

    const char* GetFolderName() const;
    const char* GetFullPath() const;

   private:
    std::array<char, MaxFilePathLength> folderPath_{};
};

tFolder::tFolder(std::array<char, MaxFilePathLength> folderPath)
    : folderPath_(folderPath) {}

const char* tFolder::GetFolderName() const {
    //TODO: add in parsing to get the name.
    return folderPath_.data();
}
const char* tFolder::GetFullPath() const {
    return folderPath_.data();
}

class FileManager {
   public:
    FileManager();

    std::vector<tFile*> GetFiles(const char* folderPath);
    std::vector<tFolder*> GetFolders();

   private:
    tSDCardDriver sdCardDriver_;
};

//global
inline FileManager fileManager;

inline FileManager::FileManager() {}

inline std::vector<tFile*> FileManager::GetFiles(const char* folderPath) {
    std::vector<tFile*> files;

    // use the sdCardDriver_ to list the files in the folderPath

    return files;
}

inline std::vector<tFolder*> FileManager::GetFolders() {
    std::vector<tFolder*> folders;

    // use the sdCardDriver_ to list the files in the folderPath

    return folders;
}

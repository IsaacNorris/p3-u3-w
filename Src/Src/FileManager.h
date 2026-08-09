#pragma once

#include <array>

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

class tFileManager {
   public:
    tFileManager();

    std::vector<tFile*> GetFiles(const char* folderPath);

   private:
    tSDCardDriver sdCardDriver_;
};

inline tFileManager::tFileManager() {}

inline std::vector<tFile*> tFileManager::GetFiles(const char* folderPath) {
    std::vector<tFile*> files;

    // use the sdCardDriver_ to list the files in the folderPath

    return files;
}

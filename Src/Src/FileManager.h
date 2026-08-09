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

   private:
};


inline tFileManager::tFileManager() {}

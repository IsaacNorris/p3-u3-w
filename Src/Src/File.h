#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "Log.h"

// Longest path the file system handles, including the null terminator.
inline constexpr size_t MaxFilePathLength = 96;
inline constexpr char PathSeparator = '/';

static_assert(MaxFilePathLength <= 256, "Path offsets are stored as uint8_t");

// Fixed size path string. Holds no handle and never allocates, so paths can be
// copied around freely and handed straight to FatFs/FS as a plain C string.
class tPath {
   public:
    tPath() = default;
    explicit tPath(const char* path) { Assign(path); }
    tPath(const char* folderPath, const char* name) {
        Assign(folderPath, name);
    }

    bool IsValid() const { return length_ != 0; }
    explicit operator bool() const { return IsValid(); }

    const char* FullPath() const { return path_.data(); }
    const char* Name() const { return path_.data() + nameOffset_; }
    const char* Extension() const;
    size_t Length() const { return length_; }

    // Case insensitive, with or without the leading dot.
    bool HasExtension(const char* extension) const;

    bool Assign(const char* path);
    bool Assign(const char* folderPath, const char* name);
    void Clear();

   private:
    std::array<char, MaxFilePathLength> path_{};
    uint8_t length_{0};
    uint8_t nameOffset_{0};

    bool Append(const char* text);
    void StripTrailingSeparators(uint8_t minimumLength);
    void Finalise();

    static char ToLower(char character);
};

inline void tPath::Clear() {
    path_[0] = '\0';
    length_ = 0;
    nameOffset_ = 0;
}

inline bool tPath::Append(const char* text) {
    if (text == nullptr) {
        return true;
    }

    size_t index = length_;
    while (*text != '\0') {
        if (index >= MaxFilePathLength - 1) {
            Log::Error("Path exceeds MaxFilePathLength");
            Clear();
            return false;
        }
        path_[index++] = *text++;
    }

    path_[index] = '\0';
    length_ = static_cast<uint8_t>(index);
    return true;
}

inline void tPath::StripTrailingSeparators(uint8_t minimumLength) {
    while (length_ > minimumLength && path_[length_ - 1] == PathSeparator) {
        path_[--length_] = '\0';
    }
}

inline void tPath::Finalise() {
    StripTrailingSeparators(1);

    nameOffset_ = 0;
    for (uint8_t index = length_; index > 0; --index) {
        if (path_[index - 1] == PathSeparator) {
            // The root separator is its own name, anything else names the tail.
            if (index < length_) {
                nameOffset_ = index;
            }
            break;
        }
    }
}

inline bool tPath::Assign(const char* path) {
    Clear();
    if (path == nullptr || *path == '\0') {
        return false;
    }
    if (!Append(path)) {
        return false;
    }
    Finalise();
    return true;
}

inline bool tPath::Assign(const char* folderPath, const char* name) {
    Clear();
    if (name == nullptr || *name == '\0') {
        return false;
    }
    if (!Append(folderPath)) {
        return false;
    }

    StripTrailingSeparators(0);
    static constexpr char Separator[] = {PathSeparator, '\0'};
    if (!Append(Separator) || !Append(name)) {
        return false;
    }

    Finalise();
    return true;
}

inline const char* tPath::Extension() const {
    for (uint8_t index = length_; index > nameOffset_; --index) {
        if (path_[index - 1] == '.') {
            return path_.data() + index;
        }
    }
    return path_.data() + length_;
}

inline bool tPath::HasExtension(const char* extension) const {
    if (extension == nullptr) {
        return false;
    }
    if (*extension == '.') {
        ++extension;
    }

    const char* own = Extension();
    while (*own != '\0' && *extension != '\0') {
        if (ToLower(*own) != ToLower(*extension)) {
            return false;
        }
        ++own;
        ++extension;
    }
    return *own == '\0' && *extension == '\0';
}

inline char tPath::ToLower(char character) {
    if (character >= 'A' && character <= 'Z') {
        return static_cast<char>(character - 'A' + 'a');
    }
    return character;
}

// Names a file on the volume. Nothing is opened by naming it, so a playlist can
// hold hundreds of these without consuming any driver handles.
class tFile {
   public:
    tFile() = default;
    explicit tFile(const char* filePath) : path_(filePath) {}
    tFile(const char* folderPath, const char* fileName)
        : path_(folderPath, fileName) {}

    const char* GetFileName() const { return path_.Name(); }
    const char* GetFullPath() const { return path_.FullPath(); }
    const char* GetExtension() const { return path_.Extension(); }
    bool HasExtension(const char* extension) const {
        return path_.HasExtension(extension);
    }

    bool IsValid() const { return path_.IsValid(); }
    explicit operator bool() const { return IsValid(); }

   private:
    tPath path_;
};

// Names a folder on the volume, under the same no-handle rules as tFile.
class tFolder {
   public:
    tFolder() = default;
    explicit tFolder(const char* folderPath) : path_(folderPath) {}
    tFolder(const char* parentPath, const char* folderName)
        : path_(parentPath, folderName) {}

    const char* GetFolderName() const { return path_.Name(); }
    const char* GetFullPath() const { return path_.FullPath(); }

    tFile FileInFolder(const char* fileName) const {
        return tFile(path_.FullPath(), fileName);
    }

    bool IsValid() const { return path_.IsValid(); }
    explicit operator bool() const { return IsValid(); }

   private:
    tPath path_;
};

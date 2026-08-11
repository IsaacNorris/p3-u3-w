#pragma once

#include <cstddef>
#include <cstdint>

#include "../Drivers/AudioDriver.h"

enum class eAudioFormat : uint8_t {
    Unknown,
    Mp3,
    Wav,
};

// What a decoder has worked out about the stream it is decoding. Anything it
// cannot know yet stays zero, so a caller has to cope with a sampleRate of 0
// until the first frame comes out.
struct tAudioStreamInfo {
    tSampleRate sampleRate{0};
    uint8_t channels{0};
    // Of the last frame decoded, which is all a VBR stream can offer.
    uint32_t bitrateKbps{0};
    // Only set by formats that state their length outright, so 0 means the
    // caller should estimate it from the file size and the bitrate.
    uint32_t durationMs{0};
};

class AudioFormat {
   public:
    // Case insensitive, with or without the leading dot, matching tPath.
    static eAudioFormat FromExtension(const char* extension);

    // Sniffs the first bytes of a file. Worth doing even when the extension
    // looks right, because a file named .mp3 is not always one.
    static eAudioFormat FromHeader(const uint8_t* data, size_t size);

    static const char* Name(eAudioFormat format);

   private:
    static bool Matches(const char* text, const char* pattern);
    static bool HasTag(const uint8_t* data, const char* tag);
};

inline bool AudioFormat::Matches(const char* text, const char* pattern) {
    if (text == nullptr) {
        return false;
    }
    if (*text == '.') {
        ++text;
    }

    while (*text != '\0' && *pattern != '\0') {
        char lowered = *text;
        if (lowered >= 'A' && lowered <= 'Z') {
            lowered = static_cast<char>(lowered - 'A' + 'a');
        }
        if (lowered != *pattern) {
            return false;
        }
        ++text;
        ++pattern;
    }

    return *text == '\0' && *pattern == '\0';
}

inline eAudioFormat AudioFormat::FromExtension(const char* extension) {
    if (Matches(extension, "mp3")) {
        return eAudioFormat::Mp3;
    }
    if (Matches(extension, "wav")) {
        return eAudioFormat::Wav;
    }
    return eAudioFormat::Unknown;
}

inline bool AudioFormat::HasTag(const uint8_t* data, const char* tag) {
    for (uint8_t index = 0; index < 4; ++index) {
        if (data[index] != static_cast<uint8_t>(tag[index])) {
            return false;
        }
    }
    return true;
}

inline eAudioFormat AudioFormat::FromHeader(const uint8_t* data, size_t size) {
    if (data == nullptr || size < 12) {
        return eAudioFormat::Unknown;
    }

    if (HasTag(data, "RIFF") && HasTag(data + 8, "WAVE")) {
        return eAudioFormat::Wav;
    }

    // Either an ID3v2 tag or a bare frame header, whose sync word is eleven set
    // bits followed by a version that is not the reserved one.
    if (data[0] == 'I' && data[1] == 'D' && data[2] == '3') {
        return eAudioFormat::Mp3;
    }
    if (data[0] == 0xFF && (data[1] & 0xE0) == 0xE0 && (data[1] & 0x18) != 0x08) {
        return eAudioFormat::Mp3;
    }

    return eAudioFormat::Unknown;
}

inline const char* AudioFormat::Name(eAudioFormat format) {
    switch (format) {
        case eAudioFormat::Mp3:
            return "MP3";
        case eAudioFormat::Wav:
            return "WAV";
        case eAudioFormat::Unknown:
            break;
    }
    return "unknown";
}

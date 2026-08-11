#pragma once

#include <cstddef>
#include <cstdint>

#include "../../../ThirdParty/minimp3/minimp3.h"
#include "../Log.h"
#include "AudioDecoder.h"

// MP3 on top of minimp3 (public domain, see ThirdParty/minimp3). minimp3 is
// header only, so its tables are instantiated by Mp3Decoder.cpp, which is the
// one part of the application that is not header only. Every target has to add
// that one file to its build.
//
// Two costs to know about before this runs on a target:
//   - mp3dec_t is about 6.7 KB and is a member here, so it lands wherever the
//     player lands rather than on a heap.
//   - mp3dec_decode_frame puts a scratch struct of about 17 KB on the stack.
//     Whatever context calls tAudioPlayer::Update needs a stack that can take
//     it, which on STM32 means growing the default main stack and on ESP32
//     means a generous stack for the task doing the decoding.
class tMp3Decoder final : public tAudioDecoder {
   public:
    tMp3Decoder() { Reset(); }

    void Reset() override;

    eResult Decode(const uint8_t* input, size_t inputSize, bool inputExhausted,
                   size_t& bytesConsumed, tPcmSample* output,
                   tFrameCount outputCapacity,
                   tFrameCount& framesProduced) override;

    const tAudioStreamInfo& StreamInfo() const override { return info_; }
    eAudioFormat Format() const override { return eAudioFormat::Mp3; }

   private:
    static constexpr size_t Id3HeaderBytes = 10;

    // Bytes the leading ID3v2 tag occupies, or 0 when there is not one.
    static size_t Id3TagBytes(const uint8_t* data, size_t size);

    static uint32_t ReadBigEndian(const uint8_t* data);
    static bool HasTag(const uint8_t* data, const char* tag);

    // Fills in durationMs from the Xing or Info header in the first frame, if
    // there is one. Without it there is no honest way to know how long a VBR
    // file runs for short of decoding all of it.
    void ParseFrameCountHeader(const uint8_t* frame, size_t size,
                               int samplesPerFrame, int sampleRate);

    mp3dec_t decoder_{};
    tAudioStreamInfo info_{};
    size_t skipRemaining_{0};
    bool tagChecked_{false};
    bool frameCountChecked_{false};
};

static_assert(MaxDecodedFrames * MaxAudioChannels >=
                  MINIMP3_MAX_SAMPLES_PER_FRAME,
              "The PCM staging buffer must hold a whole MP3 frame");

inline void tMp3Decoder::Reset() {
    mp3dec_init(&decoder_);
    info_ = tAudioStreamInfo{};
    skipRemaining_ = 0;
    tagChecked_ = false;
    frameCountChecked_ = false;
}

inline uint32_t tMp3Decoder::ReadBigEndian(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

inline bool tMp3Decoder::HasTag(const uint8_t* data, const char* tag) {
    for (uint8_t index = 0; index < 4; ++index) {
        if (data[index] != static_cast<uint8_t>(tag[index])) {
            return false;
        }
    }
    return true;
}

inline void tMp3Decoder::ParseFrameCountHeader(const uint8_t* frame,
                                               size_t size,
                                               int samplesPerFrame,
                                               int sampleRate) {
    static constexpr size_t HeaderBytes = 12;
    // Only four flag bits are defined, and no MP3 runs to ten million frames.
    // Both are here to keep a run of audio that happens to spell Xing from
    // being read as a header.
    static constexpr uint32_t KnownFlags = 0x0F;
    static constexpr uint32_t FrameCountFlag = 0x01;
    static constexpr uint32_t SaneFrameLimit = 10000000;

    if (samplesPerFrame <= 0 || sampleRate <= 0) {
        return;
    }

    // The header sits after the side info, whose length depends on the channel
    // mode and MPEG version, so it is both shorter and safer to look for it than
    // to work out where it ought to be. Xing marks VBR, Info marks CBR.
    for (size_t offset = 0; offset + HeaderBytes <= size; ++offset) {
        if (!HasTag(frame + offset, "Xing") && !HasTag(frame + offset, "Info")) {
            continue;
        }

        const uint32_t flags = ReadBigEndian(frame + offset + 4);
        if ((flags & ~KnownFlags) != 0 || (flags & FrameCountFlag) == 0) {
            return;
        }

        const uint32_t frames = ReadBigEndian(frame + offset + 8);
        if (frames == 0 || frames > SaneFrameLimit) {
            return;
        }

        info_.durationMs = static_cast<uint32_t>(
            static_cast<uint64_t>(frames) * samplesPerFrame * 1000 / sampleRate);
        return;
    }
}

inline size_t tMp3Decoder::Id3TagBytes(const uint8_t* data, size_t size) {
    if (size < Id3HeaderBytes || data[0] != 'I' || data[1] != 'D' ||
        data[2] != '3') {
        return 0;
    }

    // The size is four syncsafe bytes: seven bits each, top bit always clear.
    // A tag that breaks that rule is not one we can trust the length of, so it
    // is left to minimp3 to resync past.
    for (uint8_t index = 6; index < Id3HeaderBytes; ++index) {
        if ((data[index] & 0x80) != 0) {
            return 0;
        }
    }

    const size_t tagBytes = (static_cast<size_t>(data[6]) << 21) |
                            (static_cast<size_t>(data[7]) << 14) |
                            (static_cast<size_t>(data[8]) << 7) |
                            static_cast<size_t>(data[9]);

    // Bit 4 of the flags says a copy of the header is appended as a footer.
    const size_t footerBytes = ((data[5] & 0x10) != 0) ? Id3HeaderBytes : 0;

    return Id3HeaderBytes + tagBytes + footerBytes;
}

inline tAudioDecoder::eResult tMp3Decoder::Decode(
    const uint8_t* input, size_t inputSize, bool inputExhausted,
    size_t& bytesConsumed, tPcmSample* output, tFrameCount outputCapacity,
    tFrameCount& framesProduced) {
    bytesConsumed = 0;
    framesProduced = 0;

    if (input == nullptr || output == nullptr ||
        outputCapacity < MaxDecodedFrames) {
        Log::Error("MP3 output buffer is smaller than one frame");
        return eResult::Error;
    }

    if (!tagChecked_) {
        if (inputSize < Id3HeaderBytes) {
            return inputExhausted ? eResult::EndOfStream
                                  : eResult::NeedMoreInput;
        }
        tagChecked_ = true;
        skipRemaining_ = Id3TagBytes(input, inputSize);
    }

    // An ID3v2 tag carrying album art is routinely larger than the whole input
    // buffer, so it is eaten across as many calls as it takes. That costs a
    // read of the tag but keeps the decoder free of any need to seek.
    if (skipRemaining_ != 0) {
        bytesConsumed = (skipRemaining_ < inputSize) ? skipRemaining_ : inputSize;
        skipRemaining_ -= bytesConsumed;
        if (bytesConsumed == 0 && inputExhausted) {
            return eResult::EndOfStream;
        }
        return eResult::NeedMoreInput;
    }

    if (inputSize == 0) {
        return inputExhausted ? eResult::EndOfStream : eResult::NeedMoreInput;
    }

    // Below a full frame's worth minimp3 would only tell us to come back, so
    // wait for the buffer to fill rather than burning a decode attempt. At the
    // end of the file there is no more coming, so the short tail is decoded as
    // it is.
    if (inputSize < MaxDecoderInputChunk && !inputExhausted) {
        return eResult::NeedMoreInput;
    }

    mp3dec_frame_info_t frame{};
    const int samples = mp3dec_decode_frame(
        &decoder_, input, static_cast<int>(inputSize), output, &frame);

    if (frame.frame_bytes > 0) {
        bytesConsumed = static_cast<size_t>(frame.frame_bytes);
    }

    if (frame.frame_bytes == 0) {
        // Either the next frame straddles the end of what we have, or the file
        // has run out part way through one.
        return inputExhausted ? eResult::EndOfStream : eResult::NeedMoreInput;
    }

    if (samples == 0) {
        // Bytes were consumed resyncing past something that is not a frame, a
        // trailing ID3v1 tag being the usual one. Progress was made, so the
        // caller only has to come round again.
        return eResult::NeedMoreInput;
    }

    // minimp3 counts samples per channel, which is frames.
    framesProduced = static_cast<tFrameCount>(samples);
    info_.sampleRate = static_cast<tSampleRate>(frame.hz);
    info_.channels = static_cast<uint8_t>(frame.channels);
    info_.bitrateKbps = static_cast<uint32_t>(frame.bitrate_kbps);

    if (!frameCountChecked_) {
        frameCountChecked_ = true;
        ParseFrameCountHeader(
            input + frame.frame_offset,
            static_cast<size_t>(frame.frame_bytes - frame.frame_offset), samples,
            frame.hz);
    }

    return eResult::Produced;
}

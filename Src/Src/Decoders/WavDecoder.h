#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "../Log.h"
#include "AudioDecoder.h"

// Uncompressed PCM WAV, 8 or 16 bit, mono or stereo. There is no decoding to
// speak of here: the value of this one is that it exercises the whole streaming
// path with nothing but our own code in it, which makes it the thing to reach
// for when audio output is misbehaving and the question is whether the decoder
// or the plumbing is at fault.
class tWavDecoder final : public tAudioDecoder {
   public:
    tWavDecoder() = default;

    void Reset() override;

    eResult Decode(const uint8_t* input, size_t inputSize, bool inputExhausted,
                   size_t& bytesConsumed, tPcmSample* output,
                   tFrameCount outputCapacity,
                   tFrameCount& framesProduced) override;

    const tAudioStreamInfo& StreamInfo() const override { return info_; }
    eAudioFormat Format() const override { return eAudioFormat::Wav; }

   private:
    enum class eStage : uint8_t {
        RiffHeader,
        ChunkHeader,
        FormatChunk,
        SkipChunk,
        Data,
        Done,
    };

    static constexpr size_t RiffHeaderBytes = 12;
    static constexpr size_t ChunkHeaderBytes = 8;
    static constexpr size_t MinFormatChunkBytes = 16;
    static constexpr uint16_t PcmFormatTag = 0x0001;
    static constexpr uint16_t ExtensibleFormatTag = 0xFFFE;
    // A data chunk that says this, or says nothing, runs to the end of file.
    static constexpr uint32_t UnknownChunkSize = 0xFFFFFFFFu;

    static uint16_t ReadU16(const uint8_t* data);
    static uint32_t ReadU32(const uint8_t* data);
    static bool HasTag(const uint8_t* data, const char* tag);

    bool ParseFormat(const uint8_t* data);
    tFrameCount ConvertFrames(const uint8_t* source, tPcmSample* output,
                              tFrameCount frames) const;

    tAudioStreamInfo info_{};
    eStage stage_{eStage::RiffHeader};
    uint32_t chunkBytes_{0};
    uint32_t skipRemaining_{0};
    uint32_t dataRemaining_{0};
    uint8_t bitsPerSample_{0};
    uint8_t blockAlign_{0};
};

inline void tWavDecoder::Reset() {
    info_ = tAudioStreamInfo{};
    stage_ = eStage::RiffHeader;
    chunkBytes_ = 0;
    skipRemaining_ = 0;
    dataRemaining_ = 0;
    bitsPerSample_ = 0;
    blockAlign_ = 0;
}

inline uint16_t tWavDecoder::ReadU16(const uint8_t* data) {
    return static_cast<uint16_t>(static_cast<uint16_t>(data[0]) |
                                 (static_cast<uint16_t>(data[1]) << 8));
}

inline uint32_t tWavDecoder::ReadU32(const uint8_t* data) {
    return static_cast<uint32_t>(data[0]) |
           (static_cast<uint32_t>(data[1]) << 8) |
           (static_cast<uint32_t>(data[2]) << 16) |
           (static_cast<uint32_t>(data[3]) << 24);
}

inline bool tWavDecoder::HasTag(const uint8_t* data, const char* tag) {
    for (uint8_t index = 0; index < 4; ++index) {
        if (data[index] != static_cast<uint8_t>(tag[index])) {
            return false;
        }
    }
    return true;
}

inline bool tWavDecoder::ParseFormat(const uint8_t* data) {
    const uint16_t formatTag = ReadU16(data);
    const uint16_t channels = ReadU16(data + 2);
    const uint32_t sampleRate = ReadU32(data + 4);
    const uint16_t bitsPerSample = ReadU16(data + 14);

    if (formatTag != PcmFormatTag && formatTag != ExtensibleFormatTag) {
        Log::Error("WAV file is compressed, only PCM is supported");
        return false;
    }
    if (channels == 0 || channels > MaxAudioChannels) {
        Log::Error("WAV file has an unsupported channel count");
        return false;
    }
    if (bitsPerSample != 8 && bitsPerSample != 16) {
        Log::Error("WAV file is not 8 or 16 bit");
        return false;
    }
    if (sampleRate == 0) {
        Log::Error("WAV file has no sample rate");
        return false;
    }

    info_.sampleRate = sampleRate;
    info_.channels = static_cast<uint8_t>(channels);
    bitsPerSample_ = static_cast<uint8_t>(bitsPerSample);

    // Worked out rather than taken from the file's own block align, which some
    // writers get wrong.
    blockAlign_ = static_cast<uint8_t>(channels * (bitsPerSample / 8));
    info_.bitrateKbps =
        static_cast<uint32_t>(sampleRate * blockAlign_ * 8 / 1000);

    return true;
}

inline tFrameCount tWavDecoder::ConvertFrames(const uint8_t* source,
                                              tPcmSample* output,
                                              tFrameCount frames) const {
    const size_t samples = frames * info_.channels;

    if (bitsPerSample_ == 16) {
        // WAV samples are little endian and so is every target we build for,
        // so this is a straight copy rather than a conversion.
        std::memcpy(output, source, samples * sizeof(tPcmSample));
    } else {
        // 8 bit WAV is unsigned with 128 as silence.
        for (size_t index = 0; index < samples; ++index) {
            const int16_t centred = static_cast<int16_t>(source[index]) - 128;
            output[index] = static_cast<tPcmSample>(centred << 8);
        }
    }

    return frames;
}

inline tAudioDecoder::eResult tWavDecoder::Decode(
    const uint8_t* input, size_t inputSize, bool inputExhausted,
    size_t& bytesConsumed, tPcmSample* output, tFrameCount outputCapacity,
    tFrameCount& framesProduced) {
    bytesConsumed = 0;
    framesProduced = 0;

    if (input == nullptr || output == nullptr || outputCapacity == 0) {
        return eResult::Error;
    }

    // Header parsing walks as many chunks as the buffer holds in one call, so
    // cursor is what has been taken so far and bytesConsumed is set from it on
    // every way out.
    size_t cursor = 0;

    for (;;) {
        const size_t remaining = inputSize - cursor;

        switch (stage_) {
            case eStage::RiffHeader: {
                if (remaining < RiffHeaderBytes) {
                    bytesConsumed = cursor;
                    return inputExhausted ? eResult::Error
                                          : eResult::NeedMoreInput;
                }
                if (!HasTag(input + cursor, "RIFF") ||
                    !HasTag(input + cursor + 8, "WAVE")) {
                    Log::Error("File is not a RIFF WAVE");
                    stage_ = eStage::Done;
                    bytesConsumed = cursor;
                    return eResult::Error;
                }
                cursor += RiffHeaderBytes;
                stage_ = eStage::ChunkHeader;
                break;
            }

            case eStage::ChunkHeader: {
                if (remaining < ChunkHeaderBytes) {
                    bytesConsumed = cursor;
                    return inputExhausted ? eResult::EndOfStream
                                          : eResult::NeedMoreInput;
                }

                const uint8_t* header = input + cursor;
                chunkBytes_ = ReadU32(header + 4);
                cursor += ChunkHeaderBytes;

                if (HasTag(header, "fmt ")) {
                    stage_ = eStage::FormatChunk;
                } else if (HasTag(header, "data")) {
                    dataRemaining_ = (chunkBytes_ == 0) ? UnknownChunkSize
                                                        : chunkBytes_;
                    if (blockAlign_ == 0) {
                        Log::Error("WAV data chunk came before its format");
                        stage_ = eStage::Done;
                        bytesConsumed = cursor;
                        return eResult::Error;
                    }
                    if (dataRemaining_ != UnknownChunkSize) {
                        const uint64_t frames = dataRemaining_ / blockAlign_;
                        info_.durationMs = static_cast<uint32_t>(
                            frames * 1000 / info_.sampleRate);
                    }
                    stage_ = eStage::Data;
                } else {
                    // Chunks are padded to an even length, and the pad byte is
                    // not counted in the size.
                    skipRemaining_ = chunkBytes_ + (chunkBytes_ & 1u);
                    stage_ = eStage::SkipChunk;
                }
                break;
            }

            case eStage::FormatChunk: {
                if (chunkBytes_ < MinFormatChunkBytes) {
                    Log::Error("WAV format chunk is too short");
                    stage_ = eStage::Done;
                    bytesConsumed = cursor;
                    return eResult::Error;
                }
                if (remaining < MinFormatChunkBytes) {
                    bytesConsumed = cursor;
                    return inputExhausted ? eResult::Error
                                          : eResult::NeedMoreInput;
                }
                if (!ParseFormat(input + cursor)) {
                    stage_ = eStage::Done;
                    bytesConsumed = cursor;
                    return eResult::Error;
                }

                cursor += MinFormatChunkBytes;
                const uint32_t extra =
                    chunkBytes_ - static_cast<uint32_t>(MinFormatChunkBytes);
                skipRemaining_ = extra + (chunkBytes_ & 1u);
                stage_ = eStage::SkipChunk;
                break;
            }

            case eStage::SkipChunk: {
                const size_t take = (skipRemaining_ < remaining)
                                        ? static_cast<size_t>(skipRemaining_)
                                        : remaining;
                cursor += take;
                skipRemaining_ -= static_cast<uint32_t>(take);
                if (skipRemaining_ != 0) {
                    bytesConsumed = cursor;
                    return inputExhausted ? eResult::EndOfStream
                                          : eResult::NeedMoreInput;
                }
                stage_ = eStage::ChunkHeader;
                break;
            }

            case eStage::Data: {
                size_t usable = remaining;
                if (dataRemaining_ < usable) {
                    usable = dataRemaining_;
                }

                tFrameCount frames = usable / blockAlign_;
                if (frames > outputCapacity) {
                    frames = outputCapacity;
                }

                if (frames == 0) {
                    bytesConsumed = cursor;
                    if (dataRemaining_ == 0 || inputExhausted) {
                        stage_ = eStage::Done;
                        return eResult::EndOfStream;
                    }
                    return eResult::NeedMoreInput;
                }

                framesProduced = ConvertFrames(input + cursor, output, frames);

                const size_t taken = frames * blockAlign_;
                cursor += taken;
                if (dataRemaining_ != UnknownChunkSize) {
                    dataRemaining_ -= static_cast<uint32_t>(taken);
                    if (dataRemaining_ == 0) {
                        stage_ = eStage::Done;
                    }
                }

                bytesConsumed = cursor;
                return eResult::Produced;
            }

            case eStage::Done: {
                bytesConsumed = cursor;
                return eResult::EndOfStream;
            }
        }
    }
}

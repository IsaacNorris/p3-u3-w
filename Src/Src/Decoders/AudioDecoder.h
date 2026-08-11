#pragma once

#include <cstddef>
#include <cstdint>

#include "../../Drivers/AudioDriver.h"
#include "../AudioFormat.h"

// Most frames any decoder here emits from a single Decode call. MPEG1 Layer III
// tops out at 1152 samples per channel and WAV is clamped to the same, so one
// staging buffer sized from this fits every format.
inline constexpr tFrameCount MaxDecodedFrames = 1152;

// Longest contiguous run of input a decoder may need to make progress. The
// longest MPEG1 Layer III frame is 1441 bytes, rounded up for headroom.
inline constexpr size_t MaxDecoderInputChunk = 2048;

// Pull decoder over a byte stream. It never touches the card itself: the caller
// owns the file and hands over a window of it, which keeps the same decoder
// usable for a file, a Bluetooth stream, or a test fixture in memory.
class tAudioDecoder {
   public:
    enum class eResult : uint8_t {
        // framesProduced frames were written to output.
        Produced,
        // No frames this time. Top the buffer up and call again, and note that
        // bytesConsumed may still be non-zero because skipping a tag or
        // resyncing past damage costs bytes without producing audio.
        NeedMoreInput,
        // Nothing left to decode.
        EndOfStream,
        // The stream is not something this decoder can handle.
        Error,
    };

    virtual ~tAudioDecoder() = default;

    // Returns the decoder to where it was before the first byte of a file.
    virtual void Reset() = 0;

    // input is a contiguous view of the next inputSize bytes of the stream, and
    // inputExhausted says whether any more can still arrive. bytesConsumed is
    // always set, whatever the result.
    virtual eResult Decode(const uint8_t* input, size_t inputSize,
                           bool inputExhausted, size_t& bytesConsumed,
                           tPcmSample* output, tFrameCount outputCapacity,
                           tFrameCount& framesProduced) = 0;

    virtual const tAudioStreamInfo& StreamInfo() const = 0;
    virtual eAudioFormat Format() const = 0;
};

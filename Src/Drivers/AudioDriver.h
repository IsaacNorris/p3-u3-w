#pragma once

#include <cstddef>
#include <cstdint>

// One PCM sample for a single channel. Every target speaks 16 bit signed I2S,
// so this is the only sample format that crosses the boundary.
using tPcmSample = int16_t;

// Samples per second, per channel.
using tSampleRate = uint32_t;

// A frame is one sample for each channel, so a stereo frame is two tPcmSample.
// Every count in this interface is in frames unless the name says bytes.
using tFrameCount = size_t;

inline constexpr uint8_t MaxAudioChannels = 2;

// Platform boundary for audio out, declared here and implemented once per
// target the same way tSDCardDriver is. The driver only ever sees interleaved
// PCM, so changing decoder never reaches the hardware and another sink
// (Bluetooth A2DP, a codec behind I2C) can be dropped in behind the same calls.
//
// Nothing here blocks. The ring the platform hands to its DMA is the only place
// audio is allowed to wait, which keeps the cooperative superloop in tSystem
// free to service buttons and the display while a song plays.
//
// What a port has to provide:
//   Qt        QAudioSink pulling from a ring of our own, so the simulator
//             underruns where the hardware would instead of hiding it behind an
//             unbounded host buffer.
//   ESP32     i2s_std channel, Start configuring the slot/clock and enabling
//             it, Write as i2s_channel_write with a zero timeout.
//   STM32WB55 SAI or I2S in circular DMA over a static buffer, Write copying
//             into the half the DMA is not currently reading.
//
// Ring size is the port's call, but aim for 40-90 ms of audio: below that a
// long tick in the superloop is audible, and above it seeking and pausing start
// to feel slow. At 44.1 kHz stereo, 8 KB is about 46 ms.
class tAudioDriver {
   public:
    tAudioDriver() = default;
    ~tAudioDriver();

    tAudioDriver(const tAudioDriver&) = delete;
    tAudioDriver& operator=(const tAudioDriver&) = delete;

    // Reconfigures the hardware when the format differs from what is already
    // running and is a cheap no-op when it matches, so back to back songs at
    // the same rate never stop the stream.
    bool Start(tSampleRate sampleRate, uint8_t channels);
    void Stop();
    bool IsRunning() const;

    tSampleRate SampleRate() const;
    uint8_t Channels() const;

    // Queues at most frameCount frames and returns how many it took, which is
    // 0 when the ring is full. Callers keep the remainder and retry later.
    tFrameCount Write(const tPcmSample* interleaved, tFrameCount frameCount);

    // Queued is what is still waiting to be heard, so a song has finished
    // playing out when it reaches 0. Ring is the total capacity, which is what
    // to size a prime against.
    tFrameCount FreeFrames() const;
    tFrameCount QueuedFrames() const;
    tFrameCount RingFrames() const;

    // Stops and restarts consumption without dropping what is queued. Used for
    // pause, and to hold the output silent while the ring is primed. Both are
    // idempotent, since priming calls Pause on every tick it takes.
    void Pause();
    void Resume();
    bool IsPaused() const;

    // Drops everything queued, so skipping a song cannot leave the tail of the
    // previous one to be heard over the start of the next.
    void Flush();
};

inline tAudioDriver::~tAudioDriver() { Stop(); }

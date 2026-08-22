#include "../../../Src/Drivers/AudioDriver.h"

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QIODevice>
#include <QMediaDevices>
#include <QMutex>
#include <QMutexLocker>

#include <array>
#include <cstring>

#include "../../../Src/Src/Log.h"

// Simulator backing for audio out. The ring below stands in for the buffer an
// I2S peripheral hands to its DMA: QAudioSink pulls from it on the host's audio
// thread, which is the same relationship the hardware has with the application,
// so a tick that runs long shows up here as an underrun instead of disappearing
// into an unbounded host buffer.

namespace {

// 8192 stereo frames, about 186 ms at 44.1 kHz, matching AudioRingFrames so a
// tick that is long enough to be heard on the ESP32 or STM32 is long enough to
// be heard here.
constexpr tFrameCount RingCapacityFrames = AudioRingFrames;

// What QAudioSink reads ahead of us. Kept small so the audio dropped when a song
// is skipped is a few milliseconds rather than a noticeable tail.
constexpr qsizetype SinkBufferBytes = 4096;

class tRingSink : public QIODevice {
   public:
    void Configure(uint8_t channels) {
        QMutexLocker lock(&mutex_);
        channels_ = channels;
        read_ = 0;
        write_ = 0;
        count_ = 0;
    }

    tFrameCount Push(const tPcmSample* frames, tFrameCount frameCount) {
        QMutexLocker lock(&mutex_);

        const tFrameCount space = RingCapacityFrames - count_;
        const tFrameCount total = (frameCount < space) ? frameCount : space;
        const size_t frameBytes = FrameBytes();

        tFrameCount done = 0;
        while (done < total) {
            const tFrameCount untilEnd = RingCapacityFrames - write_;
            const tFrameCount run =
                ((total - done) < untilEnd) ? (total - done) : untilEnd;

            std::memcpy(Bytes() + (write_ * frameBytes),
                        reinterpret_cast<const uint8_t*>(frames) +
                            (done * frameBytes),
                        run * frameBytes);

            write_ = (write_ + run) % RingCapacityFrames;
            done += run;
        }

        count_ += total;
        return total;
    }

    tFrameCount Queued() const {
        QMutexLocker lock(&mutex_);
        return count_;
    }

    tFrameCount Free() const {
        QMutexLocker lock(&mutex_);
        return RingCapacityFrames - count_;
    }

    void Clear() {
        QMutexLocker lock(&mutex_);
        read_ = 0;
        write_ = 0;
        count_ = 0;
    }

    bool isSequential() const override { return true; }

    qint64 bytesAvailable() const override {
        QMutexLocker lock(&mutex_);
        return static_cast<qint64>(count_ * FrameBytes());
    }

    // An I2S stream never ends, it only goes quiet. Saying otherwise would let
    // Qt decide the device was finished the first time a tick ran long.
    bool atEnd() const override { return false; }

   protected:
    qint64 readData(char* data, qint64 maxSize) override {
        const size_t frameBytes = FrameBytes();
        const tFrameCount wanted =
            static_cast<tFrameCount>(maxSize) / frameBytes;
        if (wanted == 0) {
            return 0;
        }

        QMutexLocker lock(&mutex_);

        // Only ever hand back frames we really have. Topping a short read up
        // with silence would look like a full buffer to the host and quietly
        // slow the stream to whatever rate the ring happened to be filled at,
        // which is the opposite of the underrun we want to be able to see.
        const tFrameCount ready = (count_ < wanted) ? count_ : wanted;

        tFrameCount done = 0;
        while (done < ready) {
            const tFrameCount untilEnd = RingCapacityFrames - read_;
            const tFrameCount run =
                ((ready - done) < untilEnd) ? (ready - done) : untilEnd;

            std::memcpy(data + (done * frameBytes),
                        Bytes() + (read_ * frameBytes), run * frameBytes);

            read_ = (read_ + run) % RingCapacityFrames;
            done += run;
        }
        count_ -= ready;

        return static_cast<qint64>(ready * frameBytes);
    }

    qint64 writeData(const char*, qint64) override { return 0; }

   private:
    size_t FrameBytes() const { return channels_ * sizeof(tPcmSample); }
    uint8_t* Bytes() { return reinterpret_cast<uint8_t*>(samples_.data()); }
    const uint8_t* Bytes() const {
        return reinterpret_cast<const uint8_t*>(samples_.data());
    }

    mutable QMutex mutex_;
    std::array<tPcmSample, RingCapacityFrames * MaxAudioChannels> samples_{};
    tFrameCount read_{0};
    tFrameCount write_{0};
    tFrameCount count_{0};
    uint8_t channels_{MaxAudioChannels};
};

// Built on first use, which is after QApplication exists.
tRingSink& Ring() {
    static tRingSink ring;
    return ring;
}

QAudioSink* sink = nullptr;
tSampleRate activeSampleRate = 0;
uint8_t activeChannels = 0;
bool suspended = false;

}  // namespace

bool tAudioDriver::Start(tSampleRate sampleRate, uint8_t channels) {
    if (sampleRate == 0 || channels == 0 || channels > MaxAudioChannels) {
        Log::Error("Unsupported audio format requested");
        return false;
    }

    // Already running what was asked for, so the stream is left alone and one
    // song follows another without a gap.
    if (sink != nullptr && sampleRate == activeSampleRate &&
        channels == activeChannels) {
        return true;
    }

    Stop();

    QAudioFormat format;
    format.setSampleRate(static_cast<int>(sampleRate));
    format.setChannelCount(channels);
    format.setSampleFormat(QAudioFormat::Int16);

    const QAudioDevice device = QMediaDevices::defaultAudioOutput();
    if (device.isNull()) {
        Log::Error("No audio output device available");
        return false;
    }
    if (!device.isFormatSupported(format)) {
        Log::Error("The host will not accept this audio format");
        return false;
    }

    tRingSink& ring = Ring();
    ring.Configure(channels);
    if (!ring.isOpen() && !ring.open(QIODevice::ReadOnly)) {
        Log::Error("Failed to open the audio ring");
        return false;
    }

    sink = new QAudioSink(device, format);
    sink->setBufferSize(SinkBufferBytes);
    sink->start(&ring);

    activeSampleRate = sampleRate;
    activeChannels = channels;
    suspended = false;

    Log::Info("Audio output started");
    return true;
}

void tAudioDriver::Stop() {
    if (sink != nullptr) {
        sink->stop();
        delete sink;
        sink = nullptr;
    }

    Ring().Clear();
    activeSampleRate = 0;
    activeChannels = 0;
    suspended = false;
}

bool tAudioDriver::IsRunning() const { return sink != nullptr; }

tSampleRate tAudioDriver::SampleRate() const { return activeSampleRate; }

uint8_t tAudioDriver::Channels() const { return activeChannels; }

tFrameCount tAudioDriver::Write(const tPcmSample* interleaved,
                                tFrameCount frameCount) {
    if (sink == nullptr || interleaved == nullptr || frameCount == 0) {
        return 0;
    }
    return Ring().Push(interleaved, frameCount);
}

tFrameCount tAudioDriver::FreeFrames() const {
    return (sink == nullptr) ? 0 : Ring().Free();
}

tFrameCount tAudioDriver::QueuedFrames() const {
    return (sink == nullptr) ? 0 : Ring().Queued();
}

tFrameCount tAudioDriver::RingFrames() const { return RingCapacityFrames; }

void tAudioDriver::Pause() {
    if (sink == nullptr || suspended) {
        return;
    }
    sink->suspend();
    suspended = true;
}

void tAudioDriver::Resume() {
    if (sink == nullptr || !suspended) {
        return;
    }
    sink->resume();
    suspended = false;
}

bool tAudioDriver::IsPaused() const { return suspended; }

// Only the ring is cleared. Whatever QAudioSink has already read out of it is
// left to play, which is why the sink buffer is kept small: it bounds the audio
// that can be heard after a skip to a few milliseconds. Dropping that as well
// would mean resetting and restarting the sink, and restarting a host audio
// stream blocks for well over a hundred milliseconds, which would both stall the
// superloop and make the simulator behave nothing like the targets, where this
// is a DMA buffer reset costing next to nothing.
void tAudioDriver::Flush() { Ring().Clear(); }

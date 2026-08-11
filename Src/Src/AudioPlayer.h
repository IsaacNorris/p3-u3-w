#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "../Drivers/AudioDriver.h"
#include "../Drivers/SDCardDriver.h"
#include "../HAL/HAL.h"
#include "AudioFormat.h"
#include "Decoders/AudioDecoder.h"
#include "Decoders/Mp3Decoder.h"
#include "Decoders/WavDecoder.h"
#include "File.h"
#include "Log.h"
#include "StreamBuffer.h"

struct tVolume {
   public:
    tVolume& operator+=(tVolumeValue value) {
        SetVolume(Volume() + value);
        return *this;
    }

    tVolume& operator-=(tVolumeValue value) {
        tVolumeValue next = Volume();
        if (next < value) {
            next = 0;
        } else {
            next -= value;
        }
        SetVolume(next);
        return *this;
    }

    void SetVolume(tVolumeValue volume) {
        if (volume > VolumeMax) {
            volume_ = VolumeMax;
        } else {
            volume_ = volume;
        }
    }

    tVolumeValue Volume() const { return volume_; }

   private:
    static constexpr uint8_t VolumeMax = 100;
    tVolumeValue volume_ = {50};
};

// Compressed bytes kept ahead of the decoder. Three MP3 frames, so a frame is
// never split across a refill and the card is still read in whole blocks.
inline constexpr size_t AudioInputBufferBytes = 4096;

// One card read per tick, so this doubles as the ceiling on how fast a file can
// be pulled in. Sized for the uncompressed case: WAV at 44.1 kHz stereo eats
// 176 KB/s against MP3's 16 KB/s, so a 1 KB chunk would leave WAV with almost no
// margin on a loop that ticks slower than a couple of milliseconds. Four blocks
// is still a short enough read to not hold up the display, which shares the
// superloop.
inline constexpr size_t AudioReadChunkBytes = 2048;

// How full the platform ring is made before playback is unpaused. Priming is
// the difference between a song starting cleanly and starting with a stutter.
inline constexpr unsigned AudioPrimePercent = 75;

// How much audio the duration estimate averages over before it settles, two
// seconds at 44.1 kHz. One frame is not enough to go on: encoders routinely make
// the first frame a low bitrate header frame, which reads as a file twice its
// real length.
inline constexpr uint32_t AudioDurationEstimateFrames = 88200;

// Decode attempts per tick, not frames: most attempts that produce nothing are
// eating an ID3 tag or resyncing, and those should not cost a whole tick each.
// Only one frame can be staged at a time, so a tick decodes at most one frame.
inline constexpr uint8_t AudioDecodeAttemptsPerTick = 4;
inline constexpr uint8_t AudioDecodeAttemptsWhilePriming = 16;

// Streams one file at a time from the card, through a decoder, into the audio
// driver. Everything is sized up front and nothing here allocates, so the only
// running cost is the work Update does each tick.
//
// Both decoders are held as members. That costs the sum of their state, about
// 7 KB today, and buys a player that never allocates and never needs placement
// new. Worth revisiting only if a format turns up carrying a large table.
class tAudioPlayer {
   public:
    tAudioPlayer() = default;

    tAudioPlayer(const tAudioPlayer&) = delete;
    tAudioPlayer& operator=(const tAudioPlayer&) = delete;

    // Opens a file and arms playback. Decoding starts on the next Update, so
    // the only blocking work here is the open itself.
    bool Play(const tFile& file);

    // Gives up the file and the hardware. Pause is what to use between tracks,
    // since this powers the output down.
    void Stop();

    void Pause();
    void Resume();
    void TogglePause();

    // Pumped from the superloop. Every path through it is bounded.
    void Update();

    bool IsIdle() const { return state_ == eState::Idle; }
    bool IsPaused() const { return state_ == eState::Paused; }
    bool IsFinished() const { return state_ == eState::Finished; }
    bool IsError() const { return state_ == eState::Error; }

    // Sound is coming out, or is about to as soon as the ring is primed.
    bool IsPlaying() const {
        return state_ == eState::Priming || state_ == eState::Playing ||
               state_ == eState::Draining;
    }

    // Busy with a file, whatever phase that is in, including paused.
    bool IsActive() const { return IsPlaying() || IsPaused(); }

    const tFile& Source() const { return source_; }
    tSampleRate SampleRate() const { return sampleRate_; }
    uint8_t Channels() const { return channels_; }

    uint32_t PositionMs() const;
    uint32_t DurationMs() const { return durationMs_; }

    void SetVolume(tVolumeValue volume);
    tVolumeValue Volume() const { return volume_.Volume(); }

    // Times the ring ran dry with audio still to play. This is the number to
    // watch when adding work to the superloop: it should stay at zero.
    uint32_t Underruns() const { return underruns_; }

   private:
    enum class eState : uint8_t {
        Idle,
        // Filling the ring with the output held silent.
        Priming,
        Playing,
        Paused,
        // Nothing left to decode, waiting on the ring to empty.
        Draining,
        Finished,
        Error,
    };

    // Q15 gain, so unity is a shift rather than a divide.
    static constexpr int32_t GainShift = 15;
    static constexpr int32_t GainUnity = 1 << GainShift;

    tAudioDecoder* DecoderFor(eAudioFormat format);
    void ConfirmFormat();
    bool EnsureDriverFormat();

    void PumpPriming();
    void PumpPlaying();
    void PumpDraining();

    void FillInput();
    void PumpDecode(uint8_t attempts);
    void PumpOutput();

    void ApplyGain(tFrameCount frames, uint8_t channels);
    void UpdateDuration();

    // Lets go of the file and what was buffered from it but leaves the output
    // running, so one song can follow another without restarting the hardware.
    void Release();

    void Fail(const char* message);

    static int32_t GainFromVolume(tVolumeValue volume);

    tAudioDriver driver_;

    // The audio stream owns one of the two file handles the platform has, which
    // is what the other one in tSDCardDriver is kept clear for.
    tSDCardDriver stream_;

    tMp3Decoder mp3Decoder_;
    tWavDecoder wavDecoder_;
    tAudioDecoder* decoder_{nullptr};

    tStreamBuffer<AudioInputBufferBytes> input_;
    std::array<tPcmSample, MaxDecodedFrames * MaxAudioChannels> pcm_{};
    tFrameCount pcmFrames_{0};
    tFrameCount pcmWritten_{0};

    tFile source_;
    eState state_{eState::Idle};
    eState pausedFrom_{eState::Playing};

    bool endOfFile_{true};
    bool decoderDone_{true};
    bool formatConfirmed_{false};
    bool starved_{false};

    tSampleRate sampleRate_{0};
    uint8_t channels_{0};
    uint32_t fileBytes_{0};
    uint32_t durationMs_{0};
    uint32_t framesWritten_{0};
    uint32_t underruns_{0};

    // Ratio of compressed bytes to decoded audio, which is what the duration is
    // estimated from. Bytes spent on tags and resyncing are tracked separately
    // so they are not counted as though they were audio.
    uint32_t decodedBytes_{0};
    uint32_t decodedFrames_{0};
    uint32_t overheadBytes_{0};

    tVolume volume_;
    int32_t gain_{GainUnity};
};

inline int32_t tAudioPlayer::GainFromVolume(tVolumeValue volume) {
    if (volume >= 100) {
        return GainUnity;
    }
    // Square law, so the control sounds like it is doing the same thing at the
    // bottom of its travel as at the top. Integer only, since half the targets
    // would rather not see a float in the audio path.
    const int32_t scaled = static_cast<int32_t>(volume) * volume;
    return scaled * GainUnity / (100 * 100);
}

inline void tAudioPlayer::SetVolume(tVolumeValue volume) {
    volume_.SetVolume(volume);
    gain_ = GainFromVolume(volume_.Volume());
}

inline tAudioDecoder* tAudioPlayer::DecoderFor(eAudioFormat format) {
    switch (format) {
        case eAudioFormat::Mp3:
            return &mp3Decoder_;
        case eAudioFormat::Wav:
            return &wavDecoder_;
        case eAudioFormat::Unknown:
            break;
    }
    return nullptr;
}

inline void tAudioPlayer::Fail(const char* message) {
    Log::Error(message);
    Release();
    state_ = eState::Error;
}

inline void tAudioPlayer::Release() {
    driver_.Flush();
    stream_.Close();
    input_.Clear();
    pcmFrames_ = 0;
    pcmWritten_ = 0;
    decoder_ = nullptr;
    endOfFile_ = true;
    decoderDone_ = true;
    formatConfirmed_ = false;
    starved_ = false;
    fileBytes_ = 0;
    durationMs_ = 0;
    framesWritten_ = 0;
    decodedBytes_ = 0;
    decodedFrames_ = 0;
    overheadBytes_ = 0;
    state_ = eState::Idle;
}

inline void tAudioPlayer::Stop() {
    Release();
    source_ = tFile{};
    driver_.Stop();
    sampleRate_ = 0;
    channels_ = 0;
}

inline bool tAudioPlayer::Play(const tFile& file) {
    Release();

    if (!file.IsValid()) {
        Fail("Cannot play a file with no path");
        return false;
    }

    decoder_ = DecoderFor(AudioFormat::FromExtension(file.GetExtension()));
    if (decoder_ == nullptr) {
        Fail("Unsupported audio format");
        return false;
    }

    if (!stream_.Open(file.GetFullPath(), tSDCardDriver::eOpenMode::Read)) {
        Fail("Failed to open the audio file");
        return false;
    }

    source_ = file;
    fileBytes_ = stream_.Size();
    decoder_->Reset();
    endOfFile_ = false;
    decoderDone_ = false;
    state_ = eState::Priming;

    return true;
}

inline void tAudioPlayer::Pause() {
    if (!IsPlaying()) {
        return;
    }
    pausedFrom_ = state_;
    driver_.Pause();
    state_ = eState::Paused;
}

inline void tAudioPlayer::Resume() {
    if (state_ != eState::Paused) {
        return;
    }
    state_ = pausedFrom_;
    // Priming resumes the output itself once the ring is full enough, so it is
    // left alone here.
    if (state_ != eState::Priming) {
        driver_.Resume();
    }
}

inline void tAudioPlayer::TogglePause() {
    if (state_ == eState::Paused) {
        Resume();
    } else {
        Pause();
    }
}

inline void tAudioPlayer::Update() {
    switch (state_) {
        case eState::Priming:
            PumpPriming();
            break;
        case eState::Playing:
            PumpPlaying();
            break;
        case eState::Draining:
            PumpDraining();
            break;
        case eState::Idle:
        case eState::Paused:
        case eState::Finished:
        case eState::Error:
            break;
    }
}

inline void tAudioPlayer::ConfirmFormat() {
    if (formatConfirmed_ || decoder_ == nullptr) {
        return;
    }
    if (input_.Available() < 12 && !endOfFile_) {
        return;
    }

    formatConfirmed_ = true;

    // The extension picked the decoder, the bytes get the final say, because a
    // file named .mp3 is not always one.
    const eAudioFormat actual =
        AudioFormat::FromHeader(input_.Data(), input_.Available());
    if (actual == eAudioFormat::Unknown || actual == decoder_->Format()) {
        return;
    }

    tAudioDecoder* replacement = DecoderFor(actual);
    if (replacement == nullptr) {
        return;
    }

    Log::Warning("Audio file contents do not match its extension");
    decoder_ = replacement;
    decoder_->Reset();
}

inline bool tAudioPlayer::EnsureDriverFormat() {
    const tAudioStreamInfo& info = decoder_->StreamInfo();
    if (info.sampleRate == 0 || info.channels == 0) {
        return false;
    }
    if (driver_.IsRunning() && info.sampleRate == sampleRate_ &&
        info.channels == channels_) {
        return true;
    }

    if (driver_.IsRunning()) {
        Log::Warning("Audio format changed mid stream, restarting the output");
    }
    if (!driver_.Start(info.sampleRate, info.channels)) {
        return false;
    }

    sampleRate_ = info.sampleRate;
    channels_ = info.channels;
    return true;
}

inline void tAudioPlayer::FillInput() {
    if (endOfFile_) {
        return;
    }

    const size_t space = input_.FreeSpace();
    if (space == 0) {
        return;
    }

    const size_t wanted =
        (space < AudioReadChunkBytes) ? space : AudioReadChunkBytes;
    const size_t read = stream_.Read(input_.WritePointer(), wanted);
    input_.CommitWrite(read);

    // A short read is the only end of file either file system reports.
    if (read < wanted) {
        endOfFile_ = true;
    }
}

inline void tAudioPlayer::PumpDecode(uint8_t attempts) {
    while (attempts != 0 && pcmFrames_ == 0 && !decoderDone_) {
        --attempts;

        const size_t before = input_.Available();
        FillInput();

        size_t consumed = 0;
        tFrameCount produced = 0;
        const tAudioDecoder::eResult result =
            decoder_->Decode(input_.Data(), input_.Available(), endOfFile_,
                             consumed, pcm_.data(), MaxDecodedFrames, produced);
        input_.Consume(consumed);

        switch (result) {
            case tAudioDecoder::eResult::Produced: {
                if (produced == 0) {
                    break;
                }

                decodedBytes_ += static_cast<uint32_t>(consumed);
                decodedFrames_ += static_cast<uint32_t>(produced);
                if (decodedFrames_ <= AudioDurationEstimateFrames) {
                    UpdateDuration();
                }

                const uint8_t channels = decoder_->StreamInfo().channels;
                ApplyGain(produced, channels);
                pcmFrames_ = produced;
                pcmWritten_ = 0;
                break;
            }

            case tAudioDecoder::eResult::NeedMoreInput:
                // Tags and resync cost bytes that are not audio, so they are
                // kept out of the duration estimate.
                overheadBytes_ += static_cast<uint32_t>(consumed);

                // Nothing consumed, nothing new read and no more coming: the
                // decoder has all it will ever get and cannot use it.
                if (consumed == 0 && input_.Available() == before) {
                    if (endOfFile_) {
                        decoderDone_ = true;
                    }
                    return;
                }
                break;

            case tAudioDecoder::eResult::EndOfStream:
                decoderDone_ = true;
                break;

            case tAudioDecoder::eResult::Error:
                // Whatever is already buffered still gets played out, so a
                // damaged tail costs the end of a song rather than the song.
                Log::Error("Audio decode failed");
                decoderDone_ = true;
                break;
        }
    }
}

inline void tAudioPlayer::PumpOutput() {
    if (pcmFrames_ == 0 || channels_ == 0 || !driver_.IsRunning()) {
        return;
    }

    const tFrameCount remaining = pcmFrames_ - pcmWritten_;
    const tPcmSample* start = pcm_.data() + (pcmWritten_ * channels_);
    const tFrameCount written = driver_.Write(start, remaining);

    framesWritten_ += static_cast<uint32_t>(written);
    pcmWritten_ += written;

    if (pcmWritten_ >= pcmFrames_) {
        pcmFrames_ = 0;
        pcmWritten_ = 0;
    }
}

inline void tAudioPlayer::ApplyGain(tFrameCount frames, uint8_t channels) {
    if (gain_ >= GainUnity || channels == 0) {
        return;
    }

    const size_t samples = frames * channels;
    for (size_t index = 0; index < samples; ++index) {
        const int32_t scaled = static_cast<int32_t>(pcm_[index]) * gain_;
        pcm_[index] = static_cast<tPcmSample>(scaled >> GainShift);
    }
}

inline void tAudioPlayer::UpdateDuration() {
    const tAudioStreamInfo& info = decoder_->StreamInfo();

    // A format that states its own length is simply believed.
    if (info.durationMs != 0) {
        durationMs_ = info.durationMs;
        return;
    }

    if (decodedBytes_ == 0 || decodedFrames_ == 0 || fileBytes_ == 0 ||
        info.sampleRate == 0) {
        return;
    }

    // Scale the audio decoded so far up to the compressed bytes the whole file
    // holds. A VBR file whose later half is encoded differently reads long or
    // short by however much it differs, which is the price of not parsing a
    // Xing header, and it is only ever the number on the display.
    const uint32_t payload = (fileBytes_ > overheadBytes_)
                                 ? (fileBytes_ - overheadBytes_)
                                 : fileBytes_;
    const uint64_t frames =
        static_cast<uint64_t>(decodedFrames_) * payload / decodedBytes_;

    durationMs_ = static_cast<uint32_t>(frames * 1000 / info.sampleRate);
}

inline void tAudioPlayer::PumpPriming() {
    // The first bytes have to be in hand before the decoder is asked for
    // anything, because the extension may have picked the wrong one and a
    // decoder handed a file it does not recognise gives up on it for good.
    FillInput();
    ConfirmFormat();

    PumpDecode(AudioDecodeAttemptsWhilePriming);

    if (pcmFrames_ == 0) {
        // Nothing decoded and nothing left to try means there was no audio in
        // the file at all.
        if (decoderDone_) {
            Log::Warning("No audio could be decoded from the file");
            Release();
            state_ = eState::Finished;
        }
        return;
    }

    // Only the first decoded frame says what the format is, so the hardware
    // cannot be started, or checked against the last song, any earlier.
    if (!EnsureDriverFormat()) {
        Fail("Failed to start the audio output");
        return;
    }

    // Held silent until the ring has enough in it to survive a slow tick. The
    // ring is empty at the start of every song, whether or not the output was
    // left running by the last one, so this is not conditional on that.
    driver_.Pause();

    PumpOutput();

    const tFrameCount target = driver_.RingFrames() * AudioPrimePercent / 100;
    if (driver_.QueuedFrames() >= target || decoderDone_) {
        driver_.Resume();
        state_ = eState::Playing;
    }
}

inline void tAudioPlayer::PumpPlaying() {
    PumpDecode(AudioDecodeAttemptsPerTick);

    if (pcmFrames_ != 0 && !EnsureDriverFormat()) {
        Fail("Failed to restart the audio output");
        return;
    }

    PumpOutput();

    // Counted on the way into starvation rather than every tick spent there, so
    // one stall reads as one underrun.
    const bool starving = driver_.QueuedFrames() == 0;
    if (starving && !starved_ && !decoderDone_) {
        ++underruns_;
        Log::Warning("Audio ring ran dry");
    }
    starved_ = starving;

    if (decoderDone_ && pcmFrames_ == 0) {
        stream_.Close();
        state_ = eState::Draining;
    }
}

inline void tAudioPlayer::PumpDraining() {
    PumpOutput();

    if (pcmFrames_ == 0 && driver_.QueuedFrames() == 0) {
        // The output is deliberately left running. Whatever plays next reuses
        // it, and a song that follows at the same rate never restarts the
        // hardware, so there is no gap between tracks.
        Release();
        state_ = eState::Finished;
    }
}

inline uint32_t tAudioPlayer::PositionMs() const {
    if (sampleRate_ == 0) {
        return 0;
    }

    // What has been written less what is still sitting in the ring, so the
    // position tracks what has actually been heard.
    const uint32_t queued = static_cast<uint32_t>(driver_.QueuedFrames());
    const uint32_t played =
        (queued >= framesWritten_) ? 0 : framesWritten_ - queued;

    return static_cast<uint32_t>(static_cast<uint64_t>(played) * 1000 /
                                 sampleRate_);
}

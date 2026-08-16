#include "../../../../Src/Drivers/AudioDriver.h"

#include <cstdio>
#include <driver/dac.h>
#include <driver/i2s.h>
#include <esp_timer.h>

#include "../../../../Src/Src/Log.h"
#include "BoardPins.h"

// ESP32 backing for audio out. The I2S DMA descriptor chain is the ring: Write
// hands frames straight to the DMA with a zero timeout, so a full ring is a
// short write rather than a block, and the superloop keeps servicing buttons
// and the display while a song plays.

namespace {

constexpr i2s_port_t Port = I2S_NUM_0;

// 8 x 256 frames is 2048 frames, about 46 ms at 44.1 kHz, matching the ring the
// simulator runs so a tick that is long enough to be heard there is long enough
// to be heard here.
constexpr int DmaBufferCount = 8;
constexpr tFrameCount DmaBufferFrames = 256;
constexpr tFrameCount RingCapacityFrames = DmaBufferCount * DmaBufferFrames;

bool installed = false;
bool paused = false;
tSampleRate activeSampleRate = 0;
uint8_t activeChannels = 0;

// What has been handed to the DMA and not yet played. The legacy I2S driver
// will not say how much of the chain is still pending, so it is tracked here
// and aged off at the sample rate, which is the rate the hardware consumes it
// at. Anything that stops consumption stops the ageing with it.
tFrameCount queuedFrames = 0;
int64_t agedToUs = 0;

size_t FrameBytes() { return activeChannels * sizeof(tPcmSample); }

void ReleaseDacIfNeeded(int gpio) {
    if (gpio == 25) {
        dac_output_disable(DAC_CHANNEL_1);
    } else if (gpio == 26) {
        dac_output_disable(DAC_CHANNEL_2);
    }
}

// Drops however many frames have been clocked out since the last call. The
// leftover time is kept rather than rounded away so that a caller polling
// faster than one frame period cannot stall the count.
void AgeQueue() {
    const int64_t now = esp_timer_get_time();

    if (!installed || paused || activeSampleRate == 0) {
        agedToUs = now;
        return;
    }

    const int64_t elapsedUs = now - agedToUs;
    if (elapsedUs <= 0) {
        return;
    }

    const uint64_t played =
        static_cast<uint64_t>(elapsedUs) * activeSampleRate / 1000000;
    if (played == 0) {
        return;
    }

    agedToUs += static_cast<int64_t>(played * 1000000 / activeSampleRate);
    queuedFrames = (played >= queuedFrames)
                       ? 0
                       : queuedFrames - static_cast<tFrameCount>(played);
}

bool Install(tSampleRate sampleRate, uint8_t channels) {
    i2s_config_t config = {};
    config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
    config.sample_rate = sampleRate;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = (channels == 1) ? I2S_CHANNEL_FMT_ONLY_LEFT
                                            : I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    config.dma_buf_count = DmaBufferCount;
    config.dma_buf_len = DmaBufferFrames;
    config.use_apll = false;

    // An underrun then clocks out silence instead of replaying the descriptor
    // it ran out on, so a long tick costs a gap rather than a stutter.
    config.tx_desc_auto_clear = true;

    if (i2s_driver_install(Port, &config, 0, nullptr) != ESP_OK) {
        Log::Error("Failed to install the I2S driver");
        return false;
    }

    // GPIO 25/26 are also the ESP32 DACs. Leave those analogue drivers on and
    // BCK/LRCK never make a clean digital edge, so the PCM5102 PLL never locks.
    ReleaseDacIfNeeded(BoardPins::I2sBitClock);
    ReleaseDacIfNeeded(BoardPins::I2sWordSelect);

    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;
    pins.bck_io_num = BoardPins::I2sBitClock;
    pins.ws_io_num = BoardPins::I2sWordSelect;
    pins.data_out_num = BoardPins::I2sDataOut;
    pins.data_in_num = I2S_PIN_NO_CHANGE;

    if (i2s_set_pin(Port, &pins) != ESP_OK) {
        Log::Error("Failed to route the I2S pins");
        i2s_driver_uninstall(Port);
        return false;
    }

    return true;
}

}  // namespace

bool tAudioDriver::Start(tSampleRate sampleRate, uint8_t channels) {
    if (sampleRate == 0 || channels == 0 || channels > MaxAudioChannels) {
        Log::Error("Unsupported audio format requested");
        return false;
    }

    // Already running what was asked for, so the stream is left alone and one
    // song follows another without a gap.
    if (installed && sampleRate == activeSampleRate &&
        channels == activeChannels) {
        return true;
    }

    if (!installed) {
        if (!Install(sampleRate, channels)) {
            return false;
        }
        installed = true;
    } else {
        // Reclocking keeps the driver and its buffers, so a format change
        // costs a few register writes rather than a reinstall.
        const i2s_channel_t channelCount =
            (channels == 1) ? I2S_CHANNEL_MONO : I2S_CHANNEL_STEREO;
        if (i2s_set_clk(Port, sampleRate, I2S_BITS_PER_SAMPLE_16BIT,
                        channelCount) != ESP_OK) {
            Log::Error("Failed to reconfigure the audio output");
            return false;
        }
    }

    activeSampleRate = sampleRate;
    activeChannels = channels;

    i2s_zero_dma_buffer(Port);
    queuedFrames = 0;
    agedToUs = esp_timer_get_time();

    paused = false;
    i2s_start(Port);

    char message[96];
    std::snprintf(message, sizeof(message),
                  "Audio output started (%u Hz, %u ch, BCK %d LRCK %d DIN %d)",
                  static_cast<unsigned>(sampleRate),
                  static_cast<unsigned>(channels), BoardPins::I2sBitClock,
                  BoardPins::I2sWordSelect, BoardPins::I2sDataOut);
    Log::Info(message);
    return true;
}

void tAudioDriver::Stop() {
    if (!installed) {
        return;
    }

    i2s_zero_dma_buffer(Port);
    i2s_driver_uninstall(Port);

    installed = false;
    paused = false;
    activeSampleRate = 0;
    activeChannels = 0;
    queuedFrames = 0;
}

bool tAudioDriver::IsRunning() const { return installed; }

tSampleRate tAudioDriver::SampleRate() const { return activeSampleRate; }

uint8_t tAudioDriver::Channels() const { return activeChannels; }

tFrameCount tAudioDriver::Write(const tPcmSample* interleaved,
                                tFrameCount frameCount) {
    if (!installed || interleaved == nullptr || frameCount == 0) {
        return 0;
    }

    AgeQueue();

    size_t bytesWritten = 0;
    if (i2s_write(Port, interleaved, frameCount * FrameBytes(), &bytesWritten,
                  0) != ESP_OK) {
        return 0;
    }

    const tFrameCount written = bytesWritten / FrameBytes();
    queuedFrames += written;
    return written;
}

tFrameCount tAudioDriver::FreeFrames() const {
    if (!installed) {
        return 0;
    }
    AgeQueue();
    return RingCapacityFrames - queuedFrames;
}

tFrameCount tAudioDriver::QueuedFrames() const {
    if (!installed) {
        return 0;
    }
    AgeQueue();
    return queuedFrames;
}

tFrameCount tAudioDriver::RingFrames() const { return RingCapacityFrames; }

void tAudioDriver::Pause() {
    if (!installed || paused) {
        return;
    }

    // Clocks stay running. Stopping BCK/LRCK drops the PCM5102 PLL and the
    // analogue mute engages; the legacy I2S driver also stops recycling DMA
    // buffers, so priming writes then go nowhere. AgeQueue still freezes so
    // the ring can fill, and auto-clear clocks out silence once it is empty.
    AgeQueue();
    paused = true;
}

void tAudioDriver::Resume() {
    if (!installed || !paused) {
        return;
    }

    paused = false;
    agedToUs = esp_timer_get_time();
}

bool tAudioDriver::IsPaused() const { return paused; }

// The descriptors cannot be handed back, so what is queued is overwritten with
// silence instead. The audio that was there is gone either way, which is what
// skipping a song needs, and the few milliseconds it takes to clock the silence
// out are not audible.
void tAudioDriver::Flush() {
    if (!installed) {
        return;
    }

    i2s_zero_dma_buffer(Port);
    queuedFrames = 0;
    agedToUs = esp_timer_get_time();
}

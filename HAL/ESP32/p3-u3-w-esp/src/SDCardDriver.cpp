#include "../../../../Src/Drivers/SDCardDriver.h"

#include <SD.h>
#include <SPI.h>

#include <cstring>
#include <cstdio>

#include "../../../../Src/Src/File.h"
#include "../../../../Src/Src/Log.h"
#include "BoardPins.h"

// ESP32 backing for the SD card. The card sits on the default VSPI bus
// (SCK 18, MISO 19, MOSI 23) with its own chip select, and the open file pool
// mirrors the simulator so the application sees the same handle exhaustion
// behaviour on both targets.

namespace {

// Dupont wiring will not hold 4 MHz once I2S is also toggling. An MP3 stream
// is only tens of KB/s, so the bus can run slow and still stay ahead.
constexpr uint32_t SpiFrequencyHz = 1000000;

// Listing a directory holds the directory plus the entry being inspected, both
// of which come out of the same descriptor budget as the pool below.
constexpr uint8_t MaxDescriptors = MaxOpenFiles + 2;

// Long enough for the card to see chip select high with no clock on it, which
// is what puts it back in the idle state the initialisation sequence expects.
constexpr uint32_t RemountSettleMs = 20;

// First begin after power-on often misses the card. Two tries, not a loop.
constexpr uint8_t MountAttempts = 2;
constexpr uint32_t MountRetryMs = 250;

// VSPI's hardware CS. ss is passed as -1 so the SPI driver does not claim it,
// but a floating GPIO 5 still glitches the peripheral. Hold it high.
constexpr uint8_t UnusedVspiCs = 5;

// A remount costs the best part of a second in select timeouts, so a card that
// is genuinely gone must not be able to turn the superloop into a string of
// them. One attempt every few seconds is still often enough to pick a card back
// up that recovers on its own.
constexpr uint32_t RemountRetryMs = 3000;

// Everything needed to put a handle back after the volume has been remounted.
// The File does not survive it, but the path, the mode and where the caller had
// got to all do, so a song can carry on across one.
struct tOpenFile {
    File file;
    char path[MaxFilePathLength]{};
    tSDCardDriver::eOpenMode mode{tSDCardDriver::eOpenMode::Read};
};

tOpenFile openFiles[MaxOpenFiles];
bool mounted = false;
bool remountTried = false;
uint32_t lastRemountMs = 0;

// The FS layer only accepts absolute paths while callers pass either form, so
// every path crosses the boundary through one of these.
class tAbsolutePath {
   public:
    explicit tAbsolutePath(const char* path) {
        size_t length = 1;
        if (path != nullptr) {
            while (*path == '/') {
                ++path;
            }
            while (*path != '\0' && length < MaxFilePathLength - 1) {
                buffer_[length++] = *path++;
            }
            valid_ = *path == '\0';
        }
        buffer_[length] = '\0';
    }

    const char* Get() const { return buffer_; }
    bool IsValid() const { return valid_; }

   private:
    char buffer_[MaxFilePathLength]{'/'};
    bool valid_{true};
};

// Cores differ on whether name() is the entry or the whole path, so keep only
// the tail either way.
const char* EntryName(File& entry) {
    const char* name = entry.name();
    if (name == nullptr) {
        return "";
    }
    const char* separator = std::strrchr(name, '/');
    return separator != nullptr ? separator + 1 : name;
}

bool IsSlot(tFileHandle handle) {
    return handle >= 0 && handle < static_cast<tFileHandle>(MaxOpenFiles);
}

bool IsValidHandle(tFileHandle handle) {
    return IsSlot(handle) && static_cast<bool>(openFiles[handle].file);
}

// Nothing here touches the card, both come out of the open FIL, so this stays
// honest even once the card has stopped answering.
bool AtEndOfFile(tFileHandle handle) {
    File& file = openFiles[handle].file;
    return file.position() >= file.size();
}

const char* ModeString(tSDCardDriver::eOpenMode mode) {
    switch (mode) {
        case tSDCardDriver::eOpenMode::Write:
            return FILE_WRITE;
        case tSDCardDriver::eOpenMode::Append:
            return FILE_APPEND;
        case tSDCardDriver::eOpenMode::Read:
            break;
    }
    return FILE_READ;
}

void HoldUnusedChipSelect() {
    pinMode(UnusedVspiCs, OUTPUT);
    digitalWrite(UnusedVspiCs, HIGH);
}

// The sequence that first mounted this card. SPI.end(), dummy clocks and
// pinMode on MISO after attach all made begin fail with FR_NOT_READY, so they
// stay out of this path.
bool Begin() {
    pinMode(BoardPins::SdChipSelect, OUTPUT);
    digitalWrite(BoardPins::SdChipSelect, HIGH);
    HoldUnusedChipSelect();

    SPI.begin(BoardPins::SpiSck, BoardPins::SpiMiso, BoardPins::SpiMosi, -1);
    SPI.setHwCs(false);

    mounted = SD.begin(BoardPins::SdChipSelect, SPI, SpiFrequencyHz, "/sd",
                       MaxDescriptors);
    return mounted;
}

// CMD0 by hand so a failed begin still says whether the card is on the bus.
// 0x01 is idle (alive). 0xFF is MISO stuck high or CS not reaching the card.
// 0x00 is MISO stuck low, which is a card left in a data state.
void ProbeCard() {
    pinMode(BoardPins::SdChipSelect, OUTPUT);
    digitalWrite(BoardPins::SdChipSelect, HIGH);
    HoldUnusedChipSelect();

    SPI.begin(BoardPins::SpiSck, BoardPins::SpiMiso, BoardPins::SpiMosi, -1);
    SPI.setHwCs(false);

    SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    for (uint8_t i = 0; i < 10; ++i) {
        SPI.transfer(0xFF);
    }

    digitalWrite(BoardPins::SdChipSelect, LOW);
    uint8_t wait = 0x00;
    for (uint8_t i = 0; i < 10; ++i) {
        wait = SPI.transfer(0xFF);
        if (wait != 0x00) {
            break;
        }
    }

    const uint8_t cmd0[] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95};
    for (uint8_t i = 0; i < sizeof(cmd0); ++i) {
        SPI.transfer(cmd0[i]);
    }

    uint8_t token = 0xFF;
    for (uint8_t i = 0; i < 10; ++i) {
        token = SPI.transfer(0xFF);
        if ((token & 0x80) == 0) {
            break;
        }
    }

    digitalWrite(BoardPins::SdChipSelect, HIGH);
    SPI.transfer(0xFF);
    SPI.endTransaction();

    char message[80];
    std::snprintf(message, sizeof(message),
                  "SD probe CS %u: wait=0x%02X CMD0=0x%02X",
                  static_cast<unsigned>(BoardPins::SdChipSelect), wait, token);
    Log::Warning(message);
}

// A card that browns out, or is knocked off the bus by a marginal wire, holds
// MISO low from then on and answers every command with a 500 ms select timeout.
// Nothing in the FS layer ever re-initialises it, so the volume is torn down
// and brought back up here, which runs the card through CMD0 again and is the
// only thing that clears that state. Open handles are reopened and seeked back
// so one glitch costs a stutter rather than the song.
bool Remount() {
    if (!mounted) {
        return false;
    }

    const uint32_t now = millis();
    if (remountTried && (now - lastRemountMs) < RemountRetryMs) {
        return false;
    }
    remountTried = true;
    lastRemountMs = now;

    uint32_t positions[MaxOpenFiles] = {};
    bool reopen[MaxOpenFiles] = {};

    for (uint8_t index = 0; index < MaxOpenFiles; ++index) {
        tOpenFile& entry = openFiles[index];
        if (!entry.file) {
            continue;
        }
        reopen[index] = entry.path[0] != '\0';
        positions[index] = static_cast<uint32_t>(entry.file.position());
        entry.file.close();
        entry.file = File();
    }

    SD.end();
    mounted = false;
    delay(RemountSettleMs);

    if (!Begin()) {
        Log::Error("SD card did not come back after a remount");
        return false;
    }

    for (uint8_t index = 0; index < MaxOpenFiles; ++index) {
        if (!reopen[index]) {
            continue;
        }
        tOpenFile& entry = openFiles[index];
        entry.file = SD.open(entry.path, ModeString(entry.mode));
        if (entry.file) {
            entry.file.seek(positions[index]);
        }
    }

    Log::Warning("SD card remounted after it stopped answering");
    return true;
}

}  // namespace

bool tSDCardDriver::Mount() {
    if (mounted) {
        return true;
    }

    remountTried = false;

    for (uint8_t attempt = 0; attempt < MountAttempts; ++attempt) {
        if (attempt != 0) {
            delay(MountRetryMs);
        }
        if (Begin()) {
            Log::Info("SD card mounted");
            return true;
        }
    }

    Log::Error("Failed to mount SD card");
    ProbeCard();
    return false;
}

void tSDCardDriver::Unmount() {
    for (tOpenFile& entry : openFiles) {
        if (entry.file) {
            entry.file.close();
            entry.file = File();
        }
        entry.path[0] = '\0';
    }
    SD.end();
    mounted = false;
}

bool tSDCardDriver::IsMounted() { return mounted; }

bool tSDCardDriver::Exists(const char* path) {
    if (!mounted) {
        return false;
    }
    const tAbsolutePath absolute(path);
    return absolute.IsValid() && SD.exists(absolute.Get());
}

bool tSDCardDriver::MakeDirectory(const char* path) {
    if (!mounted) {
        return false;
    }
    const tAbsolutePath absolute(path);
    return absolute.IsValid() && SD.mkdir(absolute.Get());
}

bool tSDCardDriver::RemoveDirectory(const char* path) {
    if (!mounted) {
        return false;
    }
    const tAbsolutePath absolute(path);
    return absolute.IsValid() && SD.rmdir(absolute.Get());
}

bool tSDCardDriver::Remove(const char* path) {
    if (!mounted) {
        return false;
    }
    const tAbsolutePath absolute(path);
    return absolute.IsValid() && SD.remove(absolute.Get());
}

bool tSDCardDriver::List(const char* path, tDirectoryCallback callback,
                         void* context) {
    if (!mounted || callback == nullptr) {
        return false;
    }

    const tAbsolutePath absolute(path);
    if (!absolute.IsValid()) {
        return false;
    }

    File directory = SD.open(absolute.Get());
    if (!directory || !directory.isDirectory()) {
        return false;
    }

    for (File child = directory.openNextFile(); child;
         child = directory.openNextFile()) {
        tDirectoryEntry entry;
        entry.name = EntryName(child);
        entry.isDirectory = child.isDirectory();
        if (!entry.isDirectory) {
            entry.size = static_cast<uint32_t>(child.size());
        }

        const bool keepGoing = callback(entry, context);
        child.close();
        if (!keepGoing) {
            break;
        }
    }

    directory.close();
    return true;
}

bool tSDCardDriver::Open(const char* path, eOpenMode mode) {
    Close();

    if (!mounted || path == nullptr) {
        return false;
    }

    const tAbsolutePath absolute(path);
    if (!absolute.IsValid()) {
        return false;
    }

    tFileHandle handle = InvalidFileHandle;
    for (uint8_t index = 0; index < MaxOpenFiles; ++index) {
        if (!openFiles[index].file) {
            handle = static_cast<tFileHandle>(index);
            break;
        }
    }
    if (handle == InvalidFileHandle) {
        return false;
    }

    tOpenFile& entry = openFiles[handle];
    std::strncpy(entry.path, absolute.Get(), sizeof(entry.path) - 1);
    entry.path[sizeof(entry.path) - 1] = '\0';
    entry.mode = mode;

    // The path is recorded before the open, so a remount triggered by the open
    // failing has what it needs to put this slot back.
    entry.file = SD.open(entry.path, ModeString(mode));
    if (!entry.file && Remount()) {
        entry.file = SD.open(entry.path, ModeString(mode));
    }

    if (!entry.file || entry.file.isDirectory()) {
        if (entry.file) {
            entry.file.close();
        }
        entry.file = File();
        entry.path[0] = '\0';
        return false;
    }

    handle_ = handle;
    return true;
}

void tSDCardDriver::Close() {
    if (!IsSlot(handle_)) {
        handle_ = InvalidFileHandle;
        return;
    }

    tOpenFile& entry = openFiles[handle_];
    if (entry.file) {
        entry.file.close();
        entry.file = File();
    }
    // Cleared even when the File is already gone, since a remount that could
    // not reopen the slot leaves the path behind holding it.
    entry.path[0] = '\0';
    handle_ = InvalidFileHandle;
}

size_t tSDCardDriver::Read(void* buffer, size_t size) {
    if (!IsValidHandle(handle_) || buffer == nullptr) {
        return 0;
    }

    uint8_t* bytes = static_cast<uint8_t*>(buffer);
    const size_t read = openFiles[handle_].file.read(bytes, size);
    if (read != 0 || size == 0 || AtEndOfFile(handle_)) {
        return read;
    }

    // Nothing came back with bytes still to go, which is the card having
    // stopped answering rather than the file having run out. Remount puts the
    // handle back where it was, so the retry picks up from the same offset.
    if (!Remount() || !IsValidHandle(handle_)) {
        return 0;
    }
    return openFiles[handle_].file.read(bytes, size);
}

size_t tSDCardDriver::Write(const void* buffer, size_t size) {
    if (!IsValidHandle(handle_) || buffer == nullptr) {
        return 0;
    }
    return openFiles[handle_].file.write(static_cast<const uint8_t*>(buffer),
                                        size);
}

bool tSDCardDriver::Seek(uint32_t position) {
    if (!IsValidHandle(handle_)) {
        return false;
    }
    return openFiles[handle_].file.seek(position);
}

uint32_t tSDCardDriver::Position() const {
    if (!IsValidHandle(handle_)) {
        return 0;
    }
    return static_cast<uint32_t>(openFiles[handle_].file.position());
}

uint32_t tSDCardDriver::Size() const {
    if (!IsValidHandle(handle_)) {
        return 0;
    }
    return static_cast<uint32_t>(openFiles[handle_].file.size());
}

bool tSDCardDriver::Flush() {
    if (!IsValidHandle(handle_)) {
        return false;
    }
    openFiles[handle_].file.flush();
    return true;
}

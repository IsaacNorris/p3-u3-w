#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

// Fixed size staging buffer for compressed bytes on their way from the card to
// a decoder. A ring would be cheaper to refill, but a decoder needs one
// contiguous run of bytes to find a frame in, so unread bytes are moved to the
// front instead of wrapping. Compaction only ever moves what has not been
// decoded yet, which for MP3 is under two frames, so it costs a few hundred
// bytes of memmove per frame decoded.
template <size_t Capacity>
class tStreamBuffer {
   public:
    static constexpr size_t CapacityBytes = Capacity;

    const uint8_t* Data() const { return data_.data() + begin_; }
    size_t Available() const { return end_ - begin_; }
    size_t FreeSpace() const { return Capacity - Available(); }
    bool IsEmpty() const { return begin_ == end_; }

    // Drops bytes a decoder has taken. Over-consuming is clamped rather than
    // trusted, since a decoder reporting a bad length should not be able to
    // walk the buffer off its end.
    void Consume(size_t bytes) {
        const size_t available = Available();
        begin_ += (bytes > available) ? available : bytes;
        if (begin_ == end_) {
            Clear();
        }
    }

    // Compacts first, so the returned pointer always has FreeSpace() writable
    // bytes behind it. Follow the write with CommitWrite().
    uint8_t* WritePointer() {
        Compact();
        return data_.data() + end_;
    }

    void CommitWrite(size_t bytes) {
        const size_t room = Capacity - end_;
        end_ += (bytes > room) ? room : bytes;
    }

    void Clear() {
        begin_ = 0;
        end_ = 0;
    }

   private:
    void Compact() {
        if (begin_ == 0) {
            return;
        }

        const size_t available = Available();
        if (available != 0) {
            std::memmove(data_.data(), data_.data() + begin_, available);
        }
        begin_ = 0;
        end_ = available;
    }

    std::array<uint8_t, Capacity> data_{};
    size_t begin_{0};
    size_t end_{0};
};

#pragma once

#include <cstdint>
#include <string>

namespace bmap {

// A channel to a running elevated writer helper (bmap-writer).
class FrameChannel {
public:
    virtual ~FrameChannel() = default;

    // Send raw bytes to the helper's stdin (frame headers + payload).
    virtual bool send(const void* data, size_t len, std::string* err) = 0;

    // Signal end of input, wait for the helper to finish, and return its
    // result line ("OK" on success, "ERR <msg>" on failure).
    virtual bool finish(std::string* result, std::string* err) = 0;
};

// Launch the writer helper elevated, targeting `devicePath`. Returns nullptr
// and sets *err on failure; the caller owns the returned object.
FrameChannel* runWriterElevated(const std::string& writerPath,
                                const std::string& devicePath,
                                std::string* err);

}  // namespace bmap

#pragma once

#include <cstdint>
#include <string>

namespace bmap {

// Destination for verified image bytes. Implemented by the platform layer
// (privileged raw-device writer) and by tests (in-memory sink).
class Sink {
public:
    virtual ~Sink() = default;

    // Open the destination. Return false and set *err on failure.
    virtual bool open(std::string* err) = 0;

    // Write len bytes at byte offset. Return false and set *err on failure.
    virtual bool writeAt(uint64_t offset, const uint8_t* data, size_t len,
                         std::string* err) = 0;

    // Flush and close. Return false and set *err on failure.
    virtual bool close(std::string* err) = 0;
};

}  // namespace bmap

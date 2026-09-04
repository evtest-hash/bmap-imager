#pragma once

#include <cstdint>
#include <string>

namespace bmap {

// Minimal privileged raw-device writer: open, positional write, and sync.
// Used only by the elevated writer helper (bmap-writer).
class RawDevice {
public:
    ~RawDevice();

    bool open(const std::string& path, std::string* err);
    bool writeAt(uint64_t offset, const void* data, size_t len, std::string* err);
    bool sync(std::string* err);  // flush + fsync
    void close();

private:
#ifdef _WIN32
    void* handle_ = nullptr;  // HANDLE
#else
    int fd_ = -1;
#endif
};

}  // namespace bmap

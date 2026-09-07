#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bmap {

struct Device {
    std::string id;           // "disk2" / "sdb" / "2"
    std::string path;         // "/dev/rdisk2" / "/dev/sdb" / "\\.\PhysicalDrive2"
    uint64_t sizeBytes = 0;
    std::string description;  // model / media name
    std::string busType;      // "USB" / "Secure Digital" / "NVMe" ...
    bool removable = false;   // removable media OR external device
};

// Convert a macOS raw device path (/dev/rdiskN) to its block-device path
// (/dev/diskN). Paths that are not raw devices are returned unchanged.
inline std::string rawToBlockDevicePath(const std::string& raw) {
    const std::string prefix = "/dev/rdisk";
    if (raw.compare(0, prefix.size(), prefix) == 0) {
        return "/dev/disk" + raw.substr(prefix.size());
    }
    return raw;
}

}  // namespace bmap

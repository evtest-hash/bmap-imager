#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bmap {

// Upper bound for a flash target, to guard against accidental selection of a
// large backup drive. SD cards for embedded devices are typically 4-128 GiB.
constexpr uint64_t kMaxDeviceBytes = 256ull * 1024 * 1024 * 1024;  // 256 GiB

struct Device {
    std::string id;           // "disk2" / "sdb" / "2"
    std::string path;         // "/dev/rdisk2" / "/dev/sdb" / "\\.\PhysicalDrive2"
    uint64_t sizeBytes = 0;
    std::string description;  // model / media name
    bool removable = false;   // removable media OR external device
};

}  // namespace bmap

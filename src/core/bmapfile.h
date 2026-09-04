#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace bmap {

// One mapped region of the image, expressed in block indices (inclusive).
struct BmapRange {
    uint64_t start = 0;
    uint64_t end = 0;
    std::string checksum;  // lowercase hex SHA-256; empty when absent

    uint64_t blockCount() const { return end >= start ? end - start + 1 : 0; }
    uint64_t byteOffset(uint64_t blockSize) const { return start * blockSize; }
    uint64_t byteLength(uint64_t blockSize) const { return blockCount() * blockSize; }
};

// Parsed bmap v2.0 file.
struct BmapFile {
    std::string version;             // "2.0"
    uint64_t imageSize = 0;          // bytes
    uint64_t blockSize = 4096;       // bytes
    uint64_t blocksCount = 0;        // total blocks in image
    uint64_t mappedBlocksCount = 0;  // blocks with data
    std::string checksumType;        // "sha256"
    std::string fileChecksum;        // SHA-256 of the bmap file itself; empty when absent
    std::vector<BmapRange> ranges;   // sorted ascending by start
};

}  // namespace bmap

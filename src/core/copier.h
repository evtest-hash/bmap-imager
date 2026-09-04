#pragma once

#include "bmapfile.h"

#include <cstddef>
#include <functional>
#include <string>

namespace bmap {

class ImageSource;
class Sink;

struct CopyOptions {
    bool verifyChecksums = true;
    // progress in [0, 1]; set *cancel = true to abort between chunks.
    std::function<void(double progress, bool* cancel)> progress = nullptr;
    size_t chunkSize = 1024 * 1024;
};

class Copier {
public:
    // Copy all mapped ranges from src to sink, verifying per-range checksums.
    // Returns false and sets *err on failure or cancellation.
    static bool copy(ImageSource* src, const BmapFile& bmap, Sink* sink,
                     const CopyOptions& opts, std::string* err);
};

}  // namespace bmap

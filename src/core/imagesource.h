#pragma once

#include <cstdint>
#include <string>

namespace bmap {

// Forward-only uncompressed byte stream over a (possibly compressed) image
// file. Implemented over libarchive; reads decompressed bytes sequentially.
class ImageSource {
public:
    virtual ~ImageSource() = default;

    // Read up to len bytes. Returns bytes read (0 = EOF), or -1 on error.
    virtual int64_t read(uint8_t* buf, size_t len, std::string* err) = 0;

    // Advance to byte offset (forward-only). Returns false on error.
    virtual bool seek(uint64_t offset, std::string* err) = 0;
};

// Opens a file, transparently decompressing plain / gzip / bzip2 / xz / zstd
// via libarchive. Returns nullptr and sets *err on failure; caller owns the
// returned pointer.
ImageSource* openImageSource(const std::string& path, std::string* err);

}  // namespace bmap

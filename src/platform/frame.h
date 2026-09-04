#pragma once

#include <cstdint>
#include <string>

namespace bmap {

// Frame protocol between the GUI and the elevated writer helper (bmap-writer).
//
// GUI -> helper stdin, one write frame per mapped chunk:
//     "W <offset> <length>\n" followed by exactly <length> raw bytes.
// Terminate with "END\n".
//
// The helper writes each frame at the given byte offset, then fsyncs and
// prints exactly one result line to stdout: "OK" on success, or
// "ERR <message>" on failure, before exiting.

// Encode one write frame (header + payload) into `out`.
inline void encodeWriteFrame(uint64_t offset, const void* data, size_t len,
                             std::string* out) {
    out->clear();
    out->reserve(64 + len);
    out->append("W ");
    out->append(std::to_string(offset));
    out->append(" ");
    out->append(std::to_string(len));
    out->push_back('\n');
    out->append(static_cast<const char*>(data), len);
}

inline const char kFrameEnd[] = "END\n";

}  // namespace bmap

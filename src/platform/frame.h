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

inline const char kWFramePrefix[] = "W ";
inline const char kFrameEndToken[] = "END";
inline const char kFrameEnd[] = "END\n";  // wire form: token + newline

// Encode one write frame header ("W <offset> <length>\n"); the payload is
// sent separately so it is not copied.
inline void encodeWriteHeader(uint64_t offset, size_t len, std::string* out) {
    out->clear();
    out->append(kWFramePrefix);
    out->append(std::to_string(offset));
    out->append(" ");
    out->append(std::to_string(len));
    out->push_back('\n');
}

}  // namespace bmap

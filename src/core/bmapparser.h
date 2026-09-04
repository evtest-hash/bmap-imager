#pragma once

#include "bmapfile.h"

#include <QString>

namespace bmap {

// Parses a bmap v2.0 file. Returns false and sets *error on invalid input.
class BmapParser {
public:
    static bool parse(const QString& path, BmapFile* out, QString* error);

    // Verify the bmap file's own SHA-256 (BmapFileChecksum), when present.
    // An absent checksum passes trivially.
    static bool verifyFileChecksum(const QString& path, const BmapFile& bmap,
                                   QString* error);
};

}  // namespace bmap

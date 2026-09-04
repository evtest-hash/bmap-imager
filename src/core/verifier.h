#pragma once

#include <QByteArray>
#include <QString>
#include <cstdint>

namespace bmap {

// Lowercase hex SHA-256 of data.
QString sha256Hex(const QByteArray& data);

// Case-insensitive comparison of two hex checksum strings.
bool checksumsEqual(const QString& a, const QString& b);

}  // namespace bmap

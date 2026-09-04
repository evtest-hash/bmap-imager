#include "verifier.h"

#include <QCryptographicHash>

namespace bmap {

QString sha256Hex(const QByteArray& data) {
    const QByteArray digest =
        QCryptographicHash::hash(data, QCryptographicHash::Sha256);
    return QString::fromLatin1(digest.toHex());
}

bool checksumsEqual(const QString& a, const QString& b) {
    return QString::compare(a.trimmed(), b.trimmed(), Qt::CaseInsensitive) == 0;
}

}  // namespace bmap

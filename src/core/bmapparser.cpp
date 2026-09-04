#include "bmapparser.h"

#include "verifier.h"

#include <QCryptographicHash>
#include <QFile>
#include <QXmlStreamReader>

#include <algorithm>

namespace bmap {

namespace {

bool parseInt(const QString& s, uint64_t* out) {
    bool ok = false;
    const qulonglong v = s.trimmed().toULongLong(&ok);
    if (!ok) return false;
    *out = static_cast<uint64_t>(v);
    return true;
}

}  // namespace

bool BmapParser::parse(const QString& path, BmapFile* out, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("无法读取 bmap 文件: %1").arg(path);
        return false;
    }

    QXmlStreamReader xml(&f);

    BmapFile bmap;
    bool sawRoot = false;
    bool inBlockMap = false;
    QString currentElement;
    QString currentText;
    QString currentChecksum;

    while (!xml.atEnd()) {
        xml.readNext();

        if (xml.isStartElement()) {
            const QString name = xml.name().toString();

            if (!sawRoot) {
                if (name != QStringLiteral("bmap")) {
                    *error = QStringLiteral("不是 bmap 文件(根元素应为 <bmap>)");
                    return false;
                }
                bmap.version = xml.attributes().value("version").toString();
                const int major = bmap.version.section(QLatin1Char('.'), 0, 0).toInt();
                if (major != 2) {
                    *error = QStringLiteral("不支持的 bmap 版本: %1").arg(bmap.version);
                    return false;
                }
                sawRoot = true;
                continue;
            }

            currentElement = name;
            currentText.clear();
            if (name == QStringLiteral("BlockMap")) {
                inBlockMap = true;
            } else if (name == QStringLiteral("Range")) {
                currentChecksum = xml.attributes().value("chksum").toString();
            }
        } else if (xml.isCharacters()) {
            currentText += xml.text().toString();
        } else if (xml.isEndElement()) {
            const QString name = xml.name().toString();

            if (name == QStringLiteral("ImageSize")) {
                parseInt(currentText, &bmap.imageSize);
            } else if (name == QStringLiteral("BlockSize")) {
                parseInt(currentText, &bmap.blockSize);
            } else if (name == QStringLiteral("BlocksCount")) {
                parseInt(currentText, &bmap.blocksCount);
            } else if (name == QStringLiteral("MappedBlocksCount")) {
                parseInt(currentText, &bmap.mappedBlocksCount);
            } else if (name == QStringLiteral("ChecksumType")) {
                bmap.checksumType = currentText.trimmed().toStdString();
            } else if (name == QStringLiteral("BmapFileChecksum")) {
                bmap.fileChecksum = currentText.trimmed().toStdString();
            } else if (name == QStringLiteral("Range") && inBlockMap) {
                const QString t = currentText.trimmed();
                uint64_t start = 0;
                uint64_t end = 0;
                const int dash = t.indexOf(QLatin1Char('-'));
                bool ok = false;
                if (dash >= 0) {
                    ok = parseInt(t.left(dash), &start) &&
                         parseInt(t.mid(dash + 1), &end);
                } else {
                    ok = parseInt(t, &start);
                    end = start;
                }
                if (!ok || start > end) {
                    *error = QStringLiteral("无效的 Range: %1").arg(t);
                    return false;
                }
                BmapRange r;
                r.start = start;
                r.end = end;
                r.checksum = currentChecksum.toStdString();
                bmap.ranges.push_back(r);
            } else if (name == QStringLiteral("BlockMap")) {
                inBlockMap = false;
            }

            currentElement.clear();
            currentText.clear();
        }
    }

    if (xml.hasError()) {
        *error = QStringLiteral("XML 解析失败: %1").arg(xml.errorString());
        return false;
    }

    if (bmap.blockSize == 0) {
        *error = QStringLiteral("BlockSize 不能为 0");
        return false;
    }
    if (bmap.blocksCount == 0) {
        *error = QStringLiteral("BlocksCount 不能为 0");
        return false;
    }
    if (bmap.imageSize == 0) {
        *error = QStringLiteral("ImageSize 不能为 0");
        return false;
    }

    std::sort(bmap.ranges.begin(), bmap.ranges.end(),
              [](const BmapRange& a, const BmapRange& b) {
                  return a.start < b.start;
              });

    for (const BmapRange& r : bmap.ranges) {
        if (r.end >= bmap.blocksCount) {
            *error = QStringLiteral("Range 越界: %1-%2 (共 %3 块)")
                         .arg(r.start)
                         .arg(r.end)
                         .arg(bmap.blocksCount);
            return false;
        }
    }

    *out = std::move(bmap);
    return true;
}

bool BmapParser::verifyFileChecksum(const QString& path, const BmapFile& bmap,
                                    QString* error) {
    if (bmap.fileChecksum.empty()) {
        return true;  // nothing to verify
    }

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("无法读取 bmap 文件用于校验: %1").arg(path);
        return false;
    }
    QByteArray bytes = f.readAll();

    const QByteArray hex = QByteArray::fromStdString(bmap.fileChecksum).toLower();
    int idx = bytes.indexOf(hex);
    if (idx < 0) {
        idx = bytes.indexOf(QByteArray::fromStdString(bmap.fileChecksum).toUpper());
    }
    if (idx < 0) {
        *error = QStringLiteral("BmapFileChecksum 值未在文件中找到");
        return false;
    }
    for (int i = 0; i < hex.size(); ++i) {
        bytes[idx + i] = '0';
    }

    const QByteArray digest =
        QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    if (!checksumsEqual(QString::fromLatin1(digest.toHex()),
                        QString::fromStdString(bmap.fileChecksum))) {
        *error = QStringLiteral("bmap 文件校验和校验失败");
        return false;
    }
    return true;
}

}  // namespace bmap

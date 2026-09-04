#include "copier.h"

#include "imagesource.h"
#include "sink.h"
#include "verifier.h"

#include <QCryptographicHash>

#include <algorithm>
#include <vector>

namespace bmap {

bool Copier::copy(ImageSource* src, const BmapFile& bmap, Sink* sink,
                  const CopyOptions& opts, std::string* err) {
    const uint64_t blockSize = bmap.blockSize;

    uint64_t totalBytes = 0;
    for (const BmapRange& r : bmap.ranges) {
        totalBytes += r.byteLength(blockSize);
    }
    if (totalBytes == 0) {
        totalBytes = 1;
    }

    std::vector<uint8_t> buf(opts.chunkSize);
    uint64_t written = 0;

    for (const BmapRange& range : bmap.ranges) {
        uint64_t offset = range.byteOffset(blockSize);
        uint64_t remaining = range.byteLength(blockSize);

        if (!src->seek(offset, err)) {
            return false;
        }

        QCryptographicHash hash(QCryptographicHash::Sha256);
        const bool doVerify = opts.verifyChecksums && !range.checksum.empty();

        while (remaining > 0) {
            const size_t want =
                static_cast<size_t>(std::min<uint64_t>(buf.size(), remaining));
            const int64_t n = src->read(buf.data(), want, err);
            if (n <= 0) {
                if (n == 0) {
                    *err = "镜像在预期之前结束";
                }
                return false;
            }

            if (!sink->writeAt(offset, buf.data(), static_cast<size_t>(n), err)) {
                return false;
            }
            if (doVerify) {
                hash.addData(reinterpret_cast<const char*>(buf.data()),
                             static_cast<int>(n));
            }

            offset += static_cast<uint64_t>(n);
            remaining -= static_cast<uint64_t>(n);
            written += static_cast<uint64_t>(n);

            if (opts.progress) {
                bool cancel = false;
                opts.progress(static_cast<double>(written) /
                                  static_cast<double>(totalBytes),
                              &cancel);
                if (cancel) {
                    *err = "已取消";
                    return false;
                }
            }
        }

        if (doVerify) {
            const QString actual = QString::fromLatin1(hash.result().toHex());
            if (!checksumsEqual(actual,
                                QString::fromStdString(range.checksum))) {
                *err = "校验和校验失败(块 " + std::to_string(range.start) + ")";
                return false;
            }
        }
    }
    return true;
}

}  // namespace bmap

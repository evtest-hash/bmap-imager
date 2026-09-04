#include "flashworker.h"

#include "bmapparser.h"
#include "copier.h"
#include "imagesource.h"
#include "writersink.h"

#include <memory>
#include <utility>

namespace bmap {

FlashWorker::FlashWorker(QString imagePath, QString bmapPath, QString devicePath,
                         QString writerPath)
    : imagePath_(std::move(imagePath)),
      bmapPath_(std::move(bmapPath)),
      devicePath_(std::move(devicePath)),
      writerPath_(std::move(writerPath)) {}

void FlashWorker::cancel() { cancelled_.store(true); }

void FlashWorker::run() {
    // 1. Parse the bmap file.
    BmapFile bmap;
    QString err;
    if (!BmapParser::parse(bmapPath_, &bmap, &err)) {
        emit flashFinished(false, err);
        return;
    }
    emit logLine(QStringLiteral("parsed bmap: %1 ranges, block size %2")
                     .arg(bmap.ranges.size())
                     .arg(bmap.blockSize));

    // 2. Verify the bmap file's own checksum.
    if (!BmapParser::verifyFileChecksum(bmapPath_, bmap, &err)) {
        emit flashFinished(false, err);
        return;
    }
    emit logLine(QStringLiteral("bmap file checksum verified"));

    // 3. Open the (possibly compressed) image.
    std::string serr;
    std::unique_ptr<ImageSource> src(
        openImageSource(imagePath_.toStdString(), &serr));
    if (!src) {
        emit flashFinished(false, QString::fromStdString(serr));
        return;
    }

    // 4. Launch the elevated writer helper.
    WriterSink sink(writerPath_.toStdString(), devicePath_.toStdString());
    if (!sink.open(&serr)) {
        emit flashFinished(false, QString::fromStdString(serr));
        return;
    }
    emit logLine(QStringLiteral("writer started"));

    // 5. Copy mapped ranges, verifying checksums and reporting progress.
    CopyOptions opts;
    opts.verifyChecksums = true;
    int lastPercent = -1;
    opts.progress = [this, &lastPercent](double p, bool* cancel) {
        const int percent = static_cast<int>(p * 100.0);
        if (percent != lastPercent) {
            lastPercent = percent;
            emit progress(percent);
        }
        *cancel = cancelled_.load();
    };
    bool ok = Copier::copy(src.get(), bmap, &sink, opts, &serr);

    if (ok) {
        ok = sink.close(&serr);
    }

    if (ok) {
        emit progress(100);
        emit flashFinished(true, QStringLiteral("flash complete"));
    } else {
        emit flashFinished(false, QString::fromStdString(serr));
    }
}

}  // namespace bmap

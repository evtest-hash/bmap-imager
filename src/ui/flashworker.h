#pragma once

#include <QThread>
#include <QString>

#include <atomic>

namespace bmap {

// Runs the parse -> verify -> decompress -> verify -> write pipeline on a
// background thread, streaming verified bytes to the elevated writer helper.
class FlashWorker : public QThread {
    Q_OBJECT
public:
    FlashWorker(QString imagePath, QString bmapPath, QString devicePath,
                QString writerPath);

    void cancel();

signals:
    void progress(int percent);
    void logLine(const QString& line);
    void flashFinished(bool success, const QString& message);

protected:
    void run() override;

private:
    QString imagePath_;
    QString bmapPath_;
    QString devicePath_;
    QString writerPath_;
    std::atomic<bool> cancelled_{false};
};

}  // namespace bmap

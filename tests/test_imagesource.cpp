#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include <archive.h>

#include "imagesource.h"

class TestImageSource : public QObject {
    Q_OBJECT

    // filter: 0 = plain, 1 = gzip, 2 = bzip2
    static bool writeFiltered(const QString& path, int filter,
                              const QByteArray& data, QString* err) {
        struct archive* a = archive_write_new();
        archive_write_set_format_raw(a);
        if (filter == 1) archive_write_add_filter_gzip(a);
        else if (filter == 2) archive_write_add_filter_bzip2(a);

        const bool ok =
            archive_write_open_filename(a, path.toUtf8().constData()) == ARCHIVE_OK;
        if (ok) {
            archive_write_data(a, data.constData(), data.size());
            archive_write_close(a);
        } else {
            *err = QString::fromUtf8(archive_error_string(a));
        }
        archive_write_free(a);
        return ok;
    }

    static QByteArray pattern(int n) {
        QByteArray d;
        d.reserve(n);
        for (int i = 0; i < n; ++i) {
            d.append(char((i * 31) & 0xff));
        }
        return d;
    }

    static void readAllAndCompare(const QString& path, const QByteArray& expected) {
        std::string err;
        bmap::ImageSource* src = bmap::openImageSource(path.toStdString(), &err);
        QVERIFY2(src != nullptr, err.c_str());
        QByteArray got(expected.size(), Qt::Uninitialized);
        const qint64 n = src->read(reinterpret_cast<uint8_t*>(got.data()),
                                   expected.size(), &err);
        QCOMPARE(n, qint64(expected.size()));
        QCOMPARE(got, expected);
        delete src;
    }

private slots:
    void readsPlainFile();
    void readsGzip();
    void readsBzip2();
    void seeksForward();
};

void TestImageSource::readsPlainFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("img.bin");
    const QByteArray data = pattern(1000);
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
    f.close();
    readAllAndCompare(path, data);
}

void TestImageSource::readsGzip() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("img.gz");
    const QByteArray data = pattern(20000);
    QString err;
    QVERIFY2(writeFiltered(path, 1, data, &err), qPrintable(err));
    readAllAndCompare(path, data);
}

void TestImageSource::readsBzip2() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("img.bz2");
    const QByteArray data = pattern(20000);
    QString err;
    QVERIFY2(writeFiltered(path, 2, data, &err), qPrintable(err));
    readAllAndCompare(path, data);
}

void TestImageSource::seeksForward() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("img.bin");
    QByteArray data;
    for (int i = 0; i < 100; ++i) data.append(char(i));
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
    f.close();

    std::string err;
    bmap::ImageSource* src = bmap::openImageSource(path.toStdString(), &err);
    QVERIFY2(src != nullptr, err.c_str());

    uint8_t buf[10];
    QCOMPARE(src->read(buf, 10, &err), qint64(10));
    QVERIFY(src->seek(50, &err));
    QCOMPARE(src->read(buf, 10, &err), qint64(10));
    for (int i = 0; i < 10; ++i) {
        QCOMPARE(int(buf[i]), 50 + i);
    }
    delete src;
}

QTEST_APPLESS_MAIN(TestImageSource)
#include "test_imagesource.moc"

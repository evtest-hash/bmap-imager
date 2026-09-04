#include <QtTest>

#include <QCryptographicHash>
#include <QFile>
#include <QTemporaryDir>

#include "bmapparser.h"
#include "verifier.h"

class TestBmapParser : public QObject {
    Q_OBJECT

    static QString xml(const QString& version, const QString& blockSize,
                       const QString& blocksCount, const QString& checksum,
                       const QString& ranges) {
        return QStringLiteral(
                   "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<bmap version=\"%1\">\n"
                   "  <ImageSize> 8388608 </ImageSize>\n"
                   "  <BlockSize> %2 </BlockSize>\n"
                   "  <BlocksCount> %3 </BlocksCount>\n"
                   "  <MappedBlocksCount> 4 </MappedBlocksCount>\n"
                   "  <ChecksumType> sha256 </ChecksumType>\n"
                   "  <BmapFileChecksum> %4 </BmapFileChecksum>\n"
                   "  <BlockMap>\n%5"
                   "  </BlockMap>\n"
                   "</bmap>\n")
            .arg(version, blockSize, blocksCount, checksum, ranges);
    }

    static QString writeTemp(QTemporaryDir& dir, const QString& name,
                             const QString& content, QString* path) {
        const QString p = dir.filePath(name);
        QFile f(p);
        f.open(QIODevice::WriteOnly);
        f.write(content.toUtf8());
        f.close();
        *path = p;
        return p;
    }

    // Writes a valid bmap whose BmapFileChecksum is computed and injected.
    // Returns the checksum hex; sets *path to the file.
    static QString writeCheckedFile(QTemporaryDir& dir, QString* path) {
        const QString zeros = QString(64, QLatin1Char('0'));
        writeTemp(dir, "test.bmap",
                  xml(QStringLiteral("2.0"), QStringLiteral("4096"),
                      QStringLiteral("2048"), zeros,
                      QStringLiteral("    <Range> 0 </Range>\n")),
                  path);

        QFile f(*path);
        f.open(QIODevice::ReadOnly);
        QByteArray bytes = f.readAll();
        f.close();

        const QString hex = QString::fromLatin1(
            QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        bytes.replace(zeros.toLatin1(), hex.toLatin1());

        f.open(QIODevice::WriteOnly | QIODevice::Truncate);
        f.write(bytes);
        f.close();
        return hex;
    }

private slots:
    void parsesValidV2();
    void rejectsUnknownVersion();
    void rejectsZeroBlockSize();
    void rejectsOutOfBoundsRange();
    void sortsRangesAscending();
    void verifiesFileChecksum();
    void rejectsTamperedChecksum();
};

void TestBmapParser::parsesValidV2() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString path;
    writeTemp(dir, "test.bmap",
              xml(QStringLiteral("2.0"), QStringLiteral("4096"),
                  QStringLiteral("2048"), QString(),
                  QStringLiteral(
                      "    <Range chksum=\"aa\"> 0-1 </Range>\n"
                      "    <Range> 5 </Range>\n"
                      "    <Range> 100-101 </Range>\n")),
              &path);

    bmap::BmapFile bf;
    QString err;
    QVERIFY2(bmap::BmapParser::parse(path, &bf, &err), qPrintable(err));

    QCOMPARE(QString::fromStdString(bf.version), QStringLiteral("2.0"));
    QCOMPARE(bf.blockSize, quint64(4096));
    QCOMPARE(bf.blocksCount, quint64(2048));
    QCOMPARE(bf.ranges.size(), 3);
    QCOMPARE(QString::fromStdString(bf.ranges[0].checksum), QStringLiteral("aa"));
    QVERIFY(bf.ranges[1].checksum.empty());
}

void TestBmapParser::rejectsUnknownVersion() {
    QTemporaryDir dir;
    QString path;
    writeTemp(dir, "test.bmap",
              xml(QStringLiteral("3.0"), QStringLiteral("4096"),
                  QStringLiteral("2048"), QString(),
                  QStringLiteral("    <Range> 0 </Range>\n")),
              &path);

    bmap::BmapFile bf;
    QString err;
    QVERIFY(!bmap::BmapParser::parse(path, &bf, &err));
    QVERIFY(err.contains(QStringLiteral("version"), Qt::CaseInsensitive));
}

void TestBmapParser::rejectsZeroBlockSize() {
    QTemporaryDir dir;
    QString path;
    writeTemp(dir, "test.bmap",
              xml(QStringLiteral("2.0"), QStringLiteral("0"),
                  QStringLiteral("2048"), QString(),
                  QStringLiteral("    <Range> 0 </Range>\n")),
              &path);

    bmap::BmapFile bf;
    QString err;
    QVERIFY(!bmap::BmapParser::parse(path, &bf, &err));
}

void TestBmapParser::rejectsOutOfBoundsRange() {
    QTemporaryDir dir;
    QString path;
    writeTemp(dir, "test.bmap",
              xml(QStringLiteral("2.0"), QStringLiteral("4096"),
                  QStringLiteral("10"), QString(),
                  QStringLiteral("    <Range> 50-60 </Range>\n")),
              &path);

    bmap::BmapFile bf;
    QString err;
    QVERIFY(!bmap::BmapParser::parse(path, &bf, &err));
}

void TestBmapParser::sortsRangesAscending() {
    QTemporaryDir dir;
    QString path;
    writeTemp(dir, "test.bmap",
              xml(QStringLiteral("2.0"), QStringLiteral("4096"),
                  QStringLiteral("2048"), QString(),
                  QStringLiteral(
                      "    <Range> 100-101 </Range>\n"
                      "    <Range> 5 </Range>\n"
                      "    <Range> 0-1 </Range>\n")),
              &path);

    bmap::BmapFile bf;
    QString err;
    QVERIFY2(bmap::BmapParser::parse(path, &bf, &err), qPrintable(err));
    QCOMPARE(bf.ranges.size(), 3);
    QCOMPARE(bf.ranges[0].start, quint64(0));
    QCOMPARE(bf.ranges[1].start, quint64(5));
    QCOMPARE(bf.ranges[2].start, quint64(100));
}

void TestBmapParser::verifiesFileChecksum() {
    QTemporaryDir dir;
    QString path;
    const QString hex = writeCheckedFile(dir, &path);

    bmap::BmapFile bf;
    QString err;
    QVERIFY2(bmap::BmapParser::parse(path, &bf, &err), qPrintable(err));
    QCOMPARE(QString::fromStdString(bf.fileChecksum), hex);
    QVERIFY2(bmap::BmapParser::verifyFileChecksum(path, bf, &err), qPrintable(err));
}

void TestBmapParser::rejectsTamperedChecksum() {
    QTemporaryDir dir;
    QString path;
    writeCheckedFile(dir, &path);

    // Tamper: append a byte, changing the file hash without breaking XML.
    QFile f(path);
    f.open(QIODevice::Append);
    f.write("\n");
    f.close();

    bmap::BmapFile bf;
    QString err;
    QVERIFY2(bmap::BmapParser::parse(path, &bf, &err), qPrintable(err));
    QVERIFY(!bmap::BmapParser::verifyFileChecksum(path, bf, &err));
}

QTEST_APPLESS_MAIN(TestBmapParser)
#include "test_bmapparser.moc"

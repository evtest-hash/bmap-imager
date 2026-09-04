#include <QtTest>

#include "verifier.h"

class TestVerifier : public QObject {
    Q_OBJECT
private slots:
    void sha256KnownAnswer();
    void sha256Empty();
    void checksumsEqualCaseInsensitive();
};

void TestVerifier::sha256KnownAnswer() {
    // sha256("abc")
    QCOMPARE(bmap::sha256Hex(QByteArray("abc")),
             QStringLiteral("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
}

void TestVerifier::sha256Empty() {
    QCOMPARE(bmap::sha256Hex(QByteArray()),
             QStringLiteral("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
}

void TestVerifier::checksumsEqualCaseInsensitive() {
    QVERIFY(bmap::checksumsEqual(QStringLiteral("ABCDEF"), QStringLiteral("abcdef")));
    QVERIFY(!bmap::checksumsEqual(QStringLiteral("abc"), QStringLiteral("abd")));
}

QTEST_APPLESS_MAIN(TestVerifier)
#include "test_verifier.moc"

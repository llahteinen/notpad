#include <QTest>
#include <QFile>
#include "utils/textstream.h"


class Test_Encodings: public QObject
{
    Q_OBJECT

private slots:

    void initTestCase()
    {
        qDebug("initTestCase");
    }

    void read();
    void detectBom();
    void validateUtf();

    void cleanupTestCase()
    {
        qDebug("cleanupTestCase");
    }
};

void Test_Encodings::read()
{
    QString expected_str{QString::fromUtf8("äöÄÖ")};//fromUtf8 fromLatin1
    QString actual_str;

    {
        QFile f("testdata/test_utf8.txt");
        QVERIFY(f.open(QFile::ReadOnly));

        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Utf8);
        ts.setAutoDetectBom(true);
        actual_str = ts.readAll();
        QCOMPARE(actual_str, expected_str);
    }
    {
        QFile f("testdata/test_ansi.txt");
        QVERIFY(f.open(QFile::ReadOnly));

        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Latin1);
        actual_str = ts.readAll();
        QCOMPARE(actual_str, expected_str);
    }
    {
        QFile f("testdata/test_utf8bom.txt");
        QVERIFY(f.open(QFile::ReadOnly));

        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Utf8);
        actual_str = ts.readAll();
        QCOMPARE(actual_str, expected_str);
    }
    {
        QFile f("testdata/test_utf16bebom.txt");
        QVERIFY(f.open(QFile::ReadOnly));

        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Utf16BE);
        actual_str = ts.readAll();
        QCOMPARE(actual_str, expected_str);
    }
    {
        expected_str = QString::fromUtf8("ßàéöø");
        QFile f("testdata/test_utf16lebom.txt");
        QVERIFY(f.open(QFile::ReadOnly));

        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Utf16LE);
        ts.setAutoDetectUnicode(true);
        ts.setAutoDetectBom(true);
        qDebug() << "encoding" << ts.encoding();
        actual_str = ts.readAll();
        qDebug() << "encoding" << ts.encoding();
        QCOMPARE(actual_str, expected_str);
    }
}

void Test_Encodings::detectBom()
{
    QString expected_str{QString::fromUtf8("ßàéöø")};//fromUtf8 fromLatin1
    QString actual_str;

    {
        QFile f("testdata/test_utf8bom.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setAutoDetectBom(true);
        actual_str = ts.readAll();
        QVERIFY(ts.hasBom());
    }
    {
        QFile f("testdata/test_utf16lebom.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setAutoDetectBom(true);
        actual_str = ts.readAll();
        QVERIFY(ts.hasBom());
    }
    {
        QFile f("testdata/test_utf16bebom.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setAutoDetectBom(true);
        actual_str = ts.readAll();
        QVERIFY(ts.hasBom());
    }
    {
        QFile f("testdata/test_utf8.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setAutoDetectBom(true);
        actual_str = ts.readAll();
        QVERIFY(ts.hasBom() == false);
    }

    {
        QFile f("testdata/test_utf16bebom.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Latin1);
        ts.setAutoDetectBom(true);
        ts.setAutoDetectUnicode(false);
        actual_str = ts.readAll();
        QCOMPARE(ts.encoding(), QStringConverter::Encoding::Latin1);
    }
    {
        QFile f("testdata/test_utf16bebom.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Latin1);
        ts.setAutoDetectBom(true);
        ts.setAutoDetectUnicode(true);
        actual_str = ts.readAll();
        QCOMPARE(ts.encoding(), QStringConverter::Encoding::Utf16BE);
    }

    {
        QFile f("testdata/test_utf16bebom.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Latin1);
        ts.setAutoDetectBom(false);
        ts.setAutoDetectUnicode(true);
        actual_str = ts.readAll();
        QVERIFY(ts.hasBom() == false);
        QCOMPARE(ts.encoding(), QStringConverter::Encoding::Utf16BE);
    }
    {
        QFile f("testdata/test_utf16bebom.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Latin1);
        ts.setAutoDetectBom(false);
        ts.setAutoDetectUnicode(false);
        actual_str = ts.readAll();
        QVERIFY(ts.hasBom() == false);
        QCOMPARE(ts.encoding(), QStringConverter::Encoding::Latin1);
    }
}

void Test_Encodings::validateUtf()
{
    const QString expected_str{QString::fromUtf8("äöÄÖ")};
    QString actual_str;
    const auto fallback_encoding{QStringConverter::Encoding::Latin1};

    {
        QFile f("testdata/test_ansi.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Utf8);
        ts.setValidateUtf(true);
        actual_str = ts.readAll();
        QVERIFY(ts.hasUtfError() == TextStream::EncodingError::TRUE);
        QCOMPARE(ts.encoding(), fallback_encoding);
        QCOMPARE(actual_str, expected_str);
    }
    {
        QFile f("testdata/test_utf8.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Utf8);
        ts.setValidateUtf(true);
        actual_str = ts.readAll();
        QVERIFY(ts.hasUtfError() == TextStream::EncodingError::FALSE);
        QCOMPARE(ts.encoding(), QStringConverter::Encoding::Utf8);
        QCOMPARE(actual_str, expected_str);
    }
    {
        QFile f("testdata/test_ansi.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Latin1);
        ts.setValidateUtf(true);
        actual_str = ts.readAll();
        QCOMPARE(ts.hasUtfError(), TextStream::EncodingError::UNAVAILABLE);
        QCOMPARE(ts.encoding(), QStringConverter::Encoding::Latin1);
        QCOMPARE(actual_str, expected_str);
    }
    {
        QFile f("testdata/test_utf8.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Latin1);
        ts.setValidateUtf(true);
        actual_str = ts.readAll();
        QCOMPARE(ts.hasUtfError(), TextStream::EncodingError::UNAVAILABLE);
        QCOMPARE(ts.encoding(), QStringConverter::Encoding::Latin1);
        QCOMPARE_NE(actual_str, expected_str);
    }

    {
        QFile f("testdata/test_utf8.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Latin1);
        ts.setAutoDetectUnicode(false); /// This does not matter because the file hasn't BOM
        ts.setValidateLatin(true);
        actual_str = ts.readAll();
        QCOMPARE(ts.hasLatinError(), TextStream::EncodingError::UNAVAILABLE); /// No BOM in file, no sure way to validate
        QCOMPARE_NE(actual_str, expected_str);
    }
    {
        QFile f("testdata/test_utf8bom.txt");
        QVERIFY(f.open(QFile::ReadOnly));
        TextStream ts(&f);
        ts.setEncoding(QStringConverter::Encoding::Latin1);
        ts.setAutoDetectUnicode(false); /// Disable to keep Latin1
        ts.setValidateLatin(true);
        actual_str = ts.readAll();
        QCOMPARE(ts.hasLatinError(), TextStream::EncodingError::TRUE);
        QCOMPARE_NE(actual_str, expected_str);
    }

}

QTEST_APPLESS_MAIN(Test_Encodings)
#include "test_encodings.moc"

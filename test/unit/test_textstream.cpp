#include <QTest>
#include "utils/textstream.hpp"
#include <QPlainTextEdit>
#include <QThreadPool>

class Test_TextStream: public QObject
{
    Q_OBJECT

    std::chrono::high_resolution_clock::time_point start_t{}, end_t{};
    std::chrono::high_resolution_clock::time_point start_t2{}, end_t2{};
    std::chrono::high_resolution_clock::time_point start_t3{}, end_t3{};
    quint64 data_rec = 0;
    quint64 signal_counter = 0;
    bool finished = false;
    const quint64 filesize10MB = 10 * 1024 * 1024;

private slots:

    /// Called once before all tests
    void initTestCase()
    {
        qDebug("initTestCase");

        QString fileName = "testdata/10MB.txt";
        QFile fw;
        fw.setFileName(fileName);
        QCOMPARE(fw.open(QFile::WriteOnly | QFile::Text), true);
        qDebug() << QFileInfo(fw).absoluteFilePath();
        QTextStream ts(&fw);
        for(quint64 i = 0; i < filesize10MB; ++i)
        {
//            char c =static_cast<char>((i % 256));
            char c = static_cast<char>((i % 95) + 32); /// Plain ascii that is compatible with utf8, no special chars
            if(i % 95 == 0)
            {
                ts << '\n';
            }
            else
            {
                ts << c;
            }
        }
        fw.close();
    }

    /// Called before each test function
    void init()
    {
        qDebug("init");
        data_rec = 0;
        signal_counter = 0;
        finished = false;
        start_t = std::chrono::high_resolution_clock::now();
        end_t = start_t;
    }

    void detections();
    void readSpeed();
    void readSpeed_readAll();
    void appendSpeed();
    void readAndAppendSpeed();
    void readWithThreadPool();

    void cleanupTestCase()
    {
        qDebug("cleanupTestCase");
    }
};

/// Test that the bom and encoding etc detections work with the signal-slot read
void Test_TextStream::detections()
{
    auto onDataAvailable{[&](const QString& dataChunk, const TextStream::MetaData& meta) {
        signal_counter++;
        data_rec += dataChunk.size();
        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
        }
    }};

    {
        QString fileName = "testdata/test_utf8.txt";
        {
            QFile file(fileName);
            QVERIFY(file.open(QFile::ReadOnly));
        }

        QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
        connect(tStream.get(), &TextStream::dataAvailable, this, onDataAvailable);

        tStream->setEncoding(QStringConverter::Encoding::Utf8);
        tStream->setAutoDetectUnicode(true);
        tStream->setAutoDetectBom(true);
        tStream->setValidateUtf(true);
        tStream->setValidateLatin(true);

        tStream->readChunks();

        auto encoding = tStream->encoding();
        auto hasBom = tStream->hasBom();
        qDebug() << "encoding" << QStringConverter::nameForEncoding(encoding);
        qDebug() << "hasBom" << hasBom;

        QCOMPARE_GT(data_rec, 0);
        QCOMPARE_EQ(encoding, QStringConverter::Encoding::Utf8);
        QVERIFY(!hasBom);
    }
    init();
    {
        QString fileName = "testdata/test_utf16bebom.txt";
        {
            QFile file(fileName);
            QVERIFY(file.open(QFile::ReadOnly));
        }

        QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
        connect(tStream.get(), &TextStream::dataAvailable, this, onDataAvailable);

        tStream->setEncoding(QStringConverter::Encoding::Utf8);
        tStream->setAutoDetectUnicode(true);
        tStream->setAutoDetectBom(true);
        tStream->setValidateUtf(true);
        tStream->setValidateLatin(true);

        tStream->readChunks();

        auto encoding = tStream->encoding();
        auto hasBom = tStream->hasBom();
        qDebug() << "encoding" << QStringConverter::nameForEncoding(encoding);
        qDebug() << "hasBom" << hasBom;

        QCOMPARE_GT(data_rec, 0);
        QCOMPARE_EQ(encoding, QStringConverter::Encoding::Utf16BE);
        QVERIFY(hasBom);
    }
    init();
    {
        QString fileName = "testdata/test_ansi.txt";
        {
            QFile file(fileName);
            QVERIFY(file.open(QFile::ReadOnly));
        }

        QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
        connect(tStream.get(), &TextStream::dataAvailable, this, onDataAvailable);

        tStream->setEncoding(QStringConverter::Encoding::Utf8);
        tStream->setAutoDetectUnicode(true);
        tStream->setAutoDetectBom(true);
        tStream->setValidateUtf(true);
        tStream->setValidateLatin(true);

        tStream->readChunks();

        auto encoding = tStream->encoding();
        auto hasBom = tStream->hasBom();
        qDebug() << "encoding" << QStringConverter::nameForEncoding(encoding);
        qDebug() << "hasBom" << hasBom;

        QCOMPARE_EQ(encoding, QStringConverter::Encoding::Latin1);
        QVERIFY(!hasBom);
        QVERIFY(tStream->hasUtfError());
    }
    init();

    /// Async, get the metadata in the callback
    auto encoding = QStringConverter::Encoding::System;
    auto hasBom = false;
    TextStream::EncodingError hasUtfError{TextStream::EncodingError::UNAVAILABLE};
    TextStream::EncodingError hasLatinError{TextStream::EncodingError::UNAVAILABLE};
    auto onDataAvailableA{[&](const QString& dataChunk, const TextStream::MetaData& meta) {
        signal_counter++;
        data_rec += dataChunk.size();
        encoding = meta.encoding;
        hasBom = meta.hasBom;
        hasUtfError = meta.hasUtfError;
        hasLatinError = meta.hasLatinError;
        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
        }
    }};
    {
        /// Flip the vars to opposite of expected results to see they get updated
        encoding = QStringConverter::Encoding::System;
        hasBom = true;
        hasUtfError = TextStream::EncodingError::UNAVAILABLE;
        hasLatinError = TextStream::EncodingError::TRUE;

        QString fileName = "testdata/test_ansi.txt";
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));

        QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};

        connect(tStream.get(), &TextStream::dataAvailable, this, onDataAvailableA); /// QueuedConnection?

        tStream->setEncoding(QStringConverter::Encoding::Utf8);
        tStream->setAutoDetectUnicode(true);
        tStream->setAutoDetectBom(true);
        tStream->setValidateUtf(true); /// should turn hasUtfError from UNAVAILABLE to TRUE because input is ansi while encoding is set to Utf8
        tStream->setValidateLatin(true); /// should turn hasLatinError to UNAVAILABLE because input does not have BOM

        tStream->readChunks();

        QVERIFY(hasLatinError == TextStream::EncodingError::UNAVAILABLE); /// No BOM in file, no sure way to validate
        QVERIFY(hasUtfError == TextStream::EncodingError::TRUE);
        QCOMPARE_EQ(encoding, QStringConverter::Encoding::Latin1);
        QVERIFY(!hasBom);
        QVERIFY(tStream->hasUtfError());
    }
}

/// These tests don't assert test all that much, but they are used for testing speed of implementations
using namespace std::chrono_literals;
void Test_TextStream::readSpeed()
{
    auto thisThreadId = QThread::currentThreadId();
    Qt::HANDLE otherThreadId{};
    auto onDataAvailable{[&](const QString& dataChunk, const TextStream::MetaData& meta) {
        signal_counter++;
        data_rec += dataChunk.size();
        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            otherThreadId = QThread::currentThreadId();
            finished = true;
        }
    }};

    const auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
    QThread* thread = new QThread();
    QVERIFY(tStream->moveToThread(thread));
    connect(thread, &QThread::started, tStream.get(), &TextStream::readChunks);
    connect(tStream.get(), &TextStream::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(tStream.get(), &TextStream::dataAvailable, this, onDataAvailable, Qt::DirectConnection);
    start_t = std::chrono::high_resolution_clock::now();
    thread->start();

    auto timeout = 10000ms;
    if(!QTest::qWaitFor([&]{return finished;}, timeout))
    {
        qDebug() << "Timed out";
    }
    qInfo() << "DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";

    QCOMPARE_NE(otherThreadId, thisThreadId);
    QCOMPARE_GE((end_t-start_t).count(), 0);
    QCOMPARE_GT(signal_counter, 0);
    QCOMPARE(data_rec, filesize);
}

void Test_TextStream::readSpeed_readAll()
{
    const auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
    QString text;

    start_t = std::chrono::high_resolution_clock::now();
    text = tStream->readAll();
    end_t = std::chrono::high_resolution_clock::now();
    qInfo() << "DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";
    QCOMPARE_GE((end_t-start_t).count(), 0);
    QCOMPARE(text.size(), filesize);
}

void Test_TextStream::appendSpeed()
{
    QString testData;
    auto onDataAvailable{[&](const QString& dataChunk, const TextStream::MetaData& meta) {
        signal_counter++;
        data_rec += dataChunk.size();
        testData.append(dataChunk);
        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
        }
    }};

    const auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
    QThread* thread = new QThread();
    tStream->moveToThread(thread);
    connect(thread, &QThread::started, tStream.get(), &TextStream::readChunks);
    connect(tStream.get(), &TextStream::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(tStream.get(), &TextStream::dataAvailable, this, onDataAvailable);
    start_t = std::chrono::high_resolution_clock::now();
    thread->start();

    auto timeout = 10000ms;
    if(!QTest::qWaitFor([&]{return finished;}, timeout))
    {
        qDebug() << "Timed out";
    }
    qInfo() << "Read DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";

    QCOMPARE_GE((end_t-start_t).count(), 0);
    QCOMPARE(testData.size(), filesize10MB); /// Should be equal when the data is plain ascii
    QCOMPARE_GT(signal_counter, 0);
    QCOMPARE(data_rec, filesize);

    /// Append speed test
    std::chrono::high_resolution_clock::duration setPlainTextDur;
    {
    QPlainTextEdit editor;
    start_t = std::chrono::high_resolution_clock::now();
    editor.setPlainText(testData);
    end_t = std::chrono::high_resolution_clock::now();
    setPlainTextDur = end_t - start_t;
    qInfo() << "setPlainText DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";
    QCOMPARE(editor.document()->characterCount() - 1, testData.size()); /// I guess there will be one extra line ending or something
    }

    /// Append in blocks
//    static constexpr auto block = 1000; /// insertText about +80% to setPlainText
//    static constexpr auto block = 10000; /// Here insertText is slightly slower than the others +15%
//    static constexpr auto block = 20000; /// Here insertText about +7%
//    static constexpr auto block = 50000; /// Here insertText about +4%
    static constexpr auto block = 100000; /// Roughly same duration as setPlainText, about +1...3%. Roughly 3,5-4x slower than read on my PC
//    static constexpr auto block = 200000; /// Roughly same duration as setPlainText, often even faster
//    static constexpr auto block = 1000000; /// Slightly faster than setPlainText
//    static constexpr auto block = 10000000; /// About -4% to setPlainText
//    static constexpr auto block = 20000000; /// All at once. About -4% to setPlainText
    {
    QPlainTextEdit editor;
    start_t = std::chrono::high_resolution_clock::now();
    qsizetype blockStart = 0;
    qsizetype blockSize = block;
    quint32 iters = 0;
    while(blockStart + blockSize < testData.size())
    {
        editor.appendPlainText(testData.sliced(blockStart, blockSize)); /// This creates extra line endings
        blockStart += blockSize;
        iters++;
    }
    /// Add last partial block
    editor.appendPlainText(testData.sliced(blockStart, testData.size() - blockStart));
    iters++;
    end_t = std::chrono::high_resolution_clock::now();
    qInfo() << "appendPlainText DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";
    qInfo() << "appendPlainText iters" << iters;
    QCOMPARE(editor.document()->characterCount() - iters, testData.size()); /// There will be extra line endings
    }

    {
    QPlainTextEdit editor;
    start_t = std::chrono::high_resolution_clock::now();
    QTextCursor cursor(editor.document());
    cursor.movePosition(QTextCursor::End);
    qsizetype blockStart = 0;
    qsizetype blockSize = block;
    quint32 iters = 0;
    while(blockStart + blockSize < testData.size())
    {
        cursor.insertText(testData.sliced(blockStart, blockSize));
        blockStart += blockSize;
        iters++;
    }
    /// Add last partial block
    cursor.insertText(testData.sliced(blockStart, testData.size() - blockStart));
    iters++;
    end_t = std::chrono::high_resolution_clock::now();
    qInfo() << "insertText DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";
    qInfo() << "insertText dur%" << static_cast<double>((end_t-start_t).count()) / static_cast<double>(setPlainTextDur.count()) * 100.0 - 100.0;
    qInfo() << "insertText iters" << iters;
    QCOMPARE(editor.document()->characterCount() - 1, testData.size()); /// I guess there will be one extra line ending or something
    }
}

void Test_TextStream::readAndAppendSpeed()
{
    const auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    QPlainTextEdit editor;

    auto onDataAvailable{[&](const QString& dataChunk, const TextStream::MetaData& meta) {
        signal_counter++;
        data_rec += dataChunk.size();
        QTextCursor cursor(editor.document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(dataChunk);
        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
        }
    }};

    QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
    QThread* thread = new QThread();
    tStream->moveToThread(thread);
    connect(thread, &QThread::started, tStream.get(), &TextStream::readChunks);
    connect(tStream.get(), &TextStream::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(tStream.get(), &TextStream::dataAvailable, this, onDataAvailable);

    /// Measure total time of concurrent read file and append to QPlainTextEdit
    start_t = std::chrono::high_resolution_clock::now();
    thread->start();

    auto timeout = 10000ms;
    if(!QTest::qWaitFor([&]{return finished;}, timeout))
    {
        qDebug() << "Timed out";
    }
    qInfo() << "DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";

    QCOMPARE_GE((end_t-start_t).count(), 0);
    QCOMPARE_GT(signal_counter, 0);
    QCOMPARE(data_rec, filesize);
}

void Test_TextStream::readWithThreadPool()
{
    const auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    QPlainTextEdit editor;

    auto onDataAvailable{[&](const QString& dataChunk, const TextStream::MetaData& meta) {
        signal_counter++;
        data_rec += dataChunk.size();
        QTextCursor cursor(editor.document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(dataChunk);
        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
        }
    }};

    /// Crash if qWaitFor goes straight thru (if finished = true early)
    QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
    connect(tStream.get(), &TextStream::dataAvailable, this, onDataAvailable);

    /// Measure total time of concurrent read file and append to QPlainTextEdit
    start_t = std::chrono::high_resolution_clock::now();
    QThreadPool::globalInstance()->start([&tStream]{tStream->readChunks();}); /// Tämäkin toimii kyllä.

    auto timeout = 10000ms;
//    finished=true; /// You can cause a crash with this
    /// Don't know how the threadpool lifetime should be managed
    if(!QTest::qWaitFor([&]{return finished;}, timeout))
    {
        qDebug() << "Timed out";
    }
    qInfo() << "DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";

    QCOMPARE_GE((end_t-start_t).count(), 0);
    QCOMPARE_GT(signal_counter, 0);
    QCOMPARE(data_rec, filesize);
}

QTEST_MAIN(Test_TextStream)
#include "test_textstream.moc"

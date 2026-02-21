#include <QTest>
#include "utils/textstream.hpp"
#include <QElapsedTimer>
#include <QPlainTextEdit>
#include <QThreadPool>

using namespace std::chrono_literals;


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
    TextStream* m_tStream{};

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
        m_tStream = nullptr;
    }

    void speeeeeeeeeeeeeeed();
    void detections();
    void invalidFile();
    void readSpeed();
    void readSpeed_readAll();
    void appendSpeed();
    void readAndAppendSpeed();
    void readWithThreadPool();

    void cleanupTestCase()
    {
        qDebug("cleanupTestCase");
    }

private:
    void timerNow(std::chrono::high_resolution_clock::time_point& t)
    {
        t = std::chrono::high_resolution_clock::now();
    }
};

void Test_TextStream::speeeeeeeeeeeeeeed()
{
    QString fileName = "testdata/test_utf8.txt";
    QFile file(fileName);
    QVERIFY(file.open(QFile::ReadOnly));
    const auto ba = file.read(filesize10MB);
    const auto str = QString::fromUtf8(ba);

    QTextDocument doc1;
    QTextDocument doc2;
    const int iters = 2000;

    timerNow(start_t);
    for(int i = 0; i < iters; i++)
    {
        QTextCursor cursor(&doc1);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(str);
    }
    timerNow(end_t);

    QTextCursor cursor(&doc2);
    QTextCharFormat fmt = cursor.charFormat();
    timerNow(start_t2);
    for(int i = 0; i < iters; i++)
    {
        fmt.clearProperty(QTextFormat::ObjectType);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(str, fmt);
    }
    timerNow(end_t2);
    const auto ms1 = std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count();
    const auto ms2 = std::chrono::duration_cast<std::chrono::milliseconds>(end_t2-start_t2).count();
    qInfo() << "DONE1 in" << ms1 << "ms";
    qInfo() << "DONE2 in" << ms2 << "ms";
    qInfo() << "1 % of 2" << static_cast<double>(ms1) / ms2 * 100.0;
}

/// Test that the bom and encoding etc detections work with the signal-slot read
void Test_TextStream::detections()
{
    auto onDataQueued{[&]() {
        signal_counter++;
        QMutexLocker lock(m_tStream->queueMutex());
        data_rec += m_tStream->dataQueue().dequeue().size();
        const auto meta = m_tStream->metaQueue().dequeue();
        lock.unlock();
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
        m_tStream = tStream.get();
        tStream->setThrottlingEnabled(false);
        connect(tStream.get(), &TextStream::dataQueued, this, onDataQueued);

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
        m_tStream = tStream.get();
        connect(tStream.get(), &TextStream::dataQueued, this, onDataQueued);

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
        m_tStream = tStream.get();
        connect(tStream.get(), &TextStream::dataQueued, this, onDataQueued);

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
    auto onDataQueuedA{[&]() {
        signal_counter++;

        QMutexLocker lock(m_tStream->queueMutex());
        data_rec += m_tStream->dataQueue().dequeue().size();
        const auto meta = m_tStream->metaQueue().dequeue();
        lock.unlock();

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
        m_tStream = tStream.get();

        connect(tStream.get(), &TextStream::dataQueued, this, onDataQueuedA);

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

void Test_TextStream::invalidFile()
{
    bool error_logged = false;
    auto onDataQueued{[&]() {
        signal_counter++;

        QMutexLocker lock(m_tStream->queueMutex());

        QCOMPARE(m_tStream->dataQueue().size(), 1);
        data_rec += m_tStream->dataQueue().dequeue().size();

        QCOMPARE(m_tStream->metaQueue().size(), 1);
        const auto meta = m_tStream->metaQueue().dequeue();
        if(meta.fileError)
        {
            error_logged = true;
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
            return;
        }

        m_tStream->decrementThrottle(lock);
        lock.unlock();

        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
        }
    }};

    QString fileName = "testdata/does_not_exist.txt";

    QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
    m_tStream = tStream.get();

    QThread* thread = new QThread();
    QVERIFY(tStream->moveToThread(thread));
    connect(thread, &QThread::started, tStream.get(), &TextStream::readChunks);
    connect(tStream.get(), &TextStream::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(tStream.get(), &TextStream::dataQueued, this, onDataQueued, Qt::DirectConnection);
    start_t = std::chrono::high_resolution_clock::now();
    thread->start();

    const auto timeout = 2000ms;
    if(!QTest::qWaitFor([&]{return finished;}, timeout))
    {
        qCritical() << "Timed out";
    }
    qInfo() << "DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";

    QCOMPARE_GE((end_t-start_t).count(), 0);
    QCOMPARE_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count(), timeout.count());
    QCOMPARE(signal_counter, 1);
    QCOMPARE(data_rec, 0);
}

/// These tests don't assert test all that much, but they are used for testing speed of implementations
void Test_TextStream::readSpeed()
{
    auto thisThreadId = QThread::currentThreadId();
    Qt::HANDLE otherThreadId{};
    auto onDataQueued{[&]() {
        signal_counter++;

        QMutexLocker lock(m_tStream->queueMutex());
        data_rec += m_tStream->dataQueue().dequeue().size();
        const auto meta = m_tStream->metaQueue().dequeue();

        m_tStream->decrementThrottle(lock);
        lock.unlock();

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
    m_tStream = tStream.get();
    tStream->setThrottlingEnabled(true);

    QThread* thread = new QThread();
    QVERIFY(tStream->moveToThread(thread));
    connect(thread, &QThread::started, tStream.get(), &TextStream::readChunks);
    connect(tStream.get(), &TextStream::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(tStream.get(), &TextStream::dataQueued, this, onDataQueued, Qt::DirectConnection);
    start_t = std::chrono::high_resolution_clock::now();
    thread->start();

    auto timeout = 10000ms;
    if(!QTest::qWaitFor([&]{return finished;}, timeout))
    {
        qDebug() << "Timed out";
    }
    qInfo() << "DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";

    QVERIFY(thisThreadId);
    QVERIFY(otherThreadId);
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
    auto onDataQueued{[&]() {
        signal_counter++;

        QMutexLocker lock(m_tStream->queueMutex());
        const auto meta = m_tStream->metaQueue().dequeue();
        QString dataChunk = m_tStream->dataQueue().dequeue();
        lock.unlock();

        testData.append(dataChunk);
        data_rec += dataChunk.size();

        m_tStream->decrementThrottle();

        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
        }
    }};

    auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
    m_tStream = tStream.get();
    tStream->setThrottlingEnabled(false);

    QThread* thread = new QThread();
    tStream->moveToThread(thread);
    connect(thread, &QThread::started, tStream.get(), &TextStream::readChunks);
    connect(tStream.get(), &TextStream::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(tStream.get(), &TextStream::dataQueued, this, onDataQueued);
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

    auto onDataQueued{[&]() {
        signal_counter++;

        QMutexLocker lock(m_tStream->queueMutex());

        QElapsedTimer timer;
        timer.start();

        const auto meta = m_tStream->metaQueue().dequeue();
        QString dataChunk = m_tStream->dataQueue().dequeue();
        lock.unlock();

        QTextCursor cursor(editor.document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(dataChunk);

        const qint64 elapsed = timer.elapsed();
//        static constexpr qint64 FRAME_TIME_TARGET = 10; /// ~100 FPS
        static constexpr qint64 FRAME_TIME_TARGET = 17; /// ~60 FPS Higher update rate impacts the total time but not a lot
//        static constexpr qint64 FRAME_TIME_TARGET= 25; /// ~40 FPS
//        static constexpr qint64 FRAME_TIME_TARGET= 1000;
        int batch_size = m_tStream->batchSize();
        if(elapsed < FRAME_TIME_TARGET)
        {
            batch_size += 5;
        }
        else
        {
            batch_size -= 2;
        }
//        qInfo() << batch_size;
        m_tStream->setBatchSize(batch_size);
//        qInfo() << m_tStream->batchSize();

        m_tStream->decrementThrottle();

        data_rec += dataChunk.size();

        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
        }
    }};

    QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
    m_tStream = tStream.get();
    QThread* thread = new QThread();
    tStream->moveToThread(thread);
    connect(thread, &QThread::started, tStream.get(), &TextStream::readChunks);
    connect(tStream.get(), &TextStream::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    connect(tStream.get(), &TextStream::dataQueued, this, onDataQueued);

    /// Measure total time of concurrent read file and append to QPlainTextEdit
    start_t = std::chrono::high_resolution_clock::now();
    thread->start();

    auto timeout = 10000ms;
    if(!QTest::qWaitFor([&]{return finished;}, timeout))
    {
        qDebug() << "Timed out";
    }
    qInfo() << "DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";

    qInfo() << "Final batchSize" << m_tStream->batchSize();

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

    auto onDataQueued{[&]() {
        signal_counter++;

        QMutexLocker lock(m_tStream->queueMutex());
        const auto meta = m_tStream->metaQueue().dequeue();
        QString dataChunk = m_tStream->dataQueue().dequeue();
        lock.unlock();

        data_rec += dataChunk.size();
        QTextCursor cursor(editor.document());
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(dataChunk);

        m_tStream->decrementThrottle();

        if(meta.done)
        {
            end_t = std::chrono::high_resolution_clock::now();
            finished = true;
        }
    }};

    /// Crash if qWaitFor goes straight thru (if finished = true early)
    QScopedPointer<TextStream, QScopedPointerDeleteLater> tStream{new TextStream(fileName)};
    m_tStream = tStream.get();
    connect(tStream.get(), &TextStream::dataQueued, this, onDataQueued);

    /// Measure total time of concurrent read file and append to QPlainTextEdit
    start_t = std::chrono::high_resolution_clock::now();
    QThreadPool::globalInstance()->start([&tStream]{tStream->readChunks();}); /// Tämäkin toimii kyllä.

    auto timeout = 10000ms;
    if(!QTest::qWaitFor([&]{return finished;}, timeout))
    {
        qDebug() << "Timed out";
    }

    /// Don't call TextStream::quit() because it assumes it's running in its own thread
//    QMutexLocker lock(m_tStream->queueMutex());
//    m_tStream->m_quitCalled = true;
//    lock.unlock();
//    m_tStream->m_dataWait.notify_all();
    qInfo() << "DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(end_t-start_t).count() << "ms";
    QThreadPool::globalInstance()->waitForDone(1000);

    QCOMPARE_GE((end_t-start_t).count(), 0);
    QCOMPARE_GT(signal_counter, 0);
    QCOMPARE(data_rec, filesize);
}

QTEST_MAIN(Test_TextStream)
#include "test_textstream.moc"

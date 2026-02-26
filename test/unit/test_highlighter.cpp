#include <QPlainTextEdit>
#include <QSignalSpy>
#include <QTest>
#include <utils/textstream.hpp>
#include "utils/highlighter.hpp"


using namespace std::chrono_literals;


class Test_Highlighter: public QObject
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

    void speeeeeeeeeeeeeeed();

    void cleanup()
    {
        qDebug("cleanup");
    }

    void cleanupTestCase()
    {
        qDebug("cleanupTestCase");
    }

private:
    void timerNow(std::chrono::high_resolution_clock::time_point& t)
    {
        t = std::chrono::high_resolution_clock::now();
    }
    void printTime(const QString& text, std::chrono::high_resolution_clock::time_point& start, std::chrono::high_resolution_clock::time_point& end)
    {
        qInfo() << text << "in" << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "ms";
    }
};

void Test_Highlighter::speeeeeeeeeeeeeeed()
{
    auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    TextStream stream(fileName);
    QPlainTextEdit editor;
    editor.setPlainText(stream.readAll());
    QCOMPARE_EQ(editor.document()->characterCount(), filesize+1);

    Highlighter lighter;
    connect(&lighter, &Highlighter::rehighlightFinished, this, [this]{ timerNow(end_t); });
    QSignalSpy spy(&lighter, &Highlighter::rehighlightFinished);

    timerNow(start_t);
    lighter.setDocument(editor.document());

    QTRY_VERIFY(spy.count() == 1);

    printTime("Highlighter DONE", start_t, end_t);

    timerNow(start_t);
    lighter.setRegex("b", {});
    QTRY_VERIFY(spy.count() == 2);
    printTime("Highlighter DONE", start_t, end_t);
}


QTEST_MAIN(Test_Highlighter)
#include "test_highlighter.moc"

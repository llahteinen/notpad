#include <QSignalSpy>
#include <QTest>
#include "testutils/testutils.hpp"
#include "editor.hpp"
#include "utils/search.hpp"
#include "utils/regex.hpp"


class Test_Search : public QObject
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

        TestUtils::generateAsciiFile(filesize10MB, "testdata/10MB.txt");
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

    void countMatches();
    void getMatchCount_editSearchTerm();
    void getMatchCount_editDoc();

    void cleanup()
    {
        qDebug("cleanup");
    }

    void cleanupTestCase()
    {
        qDebug("cleanupTestCase");
    }

private:
};

using namespace TestUtils;


void Test_Search::countMatches()
{
    auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    TextStream stream(fileName);
    Editor editor;
    editor.setPlainText(stream.readAll());
    QCOMPARE_EQ(editor.document()->characterCount(), filesize+1);

    int matches;
    QString doc;
    QString sterm;
    QRegularExpression reg;
    QTextDocument::FindFlags flags;

    timerNow(start_t);
    doc = editor.document()->toRawText();
    sterm = "ABC";
    flags = {};
    reg = Regex::stringToRegex(sterm, flags);
    matches = Search::countMatches(doc, sterm, reg, flags);
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 220753); /// Case insensitive 220753

    timerNow(start_t);
    doc = editor.document()->toRawText();
    sterm = "ABD";
    flags = {};
    reg = Regex::stringToRegex(sterm, flags);
    matches = Search::countMatches(doc, sterm, reg, flags);
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 0);

    timerNow(start_t);
    doc = editor.document()->toRawText();
    sterm = "bcd";
    flags = {};
    reg = Regex::stringToRegex(sterm, flags);
    matches = Search::countMatches(doc, sterm, reg, flags);
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 220753);

    timerNow(start_t);
    doc = editor.document()->toRawText();
    sterm = "bcd";
    flags = QTextDocument::FindFlag::FindCaseSensitively;
    reg = Regex::stringToRegex(sterm, flags);
    matches = Search::countMatches(doc, sterm, reg, flags);
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 110376);
}

void Test_Search::getMatchCount_editSearchTerm()
{
    auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    TextStream stream(fileName);
    Editor editor;
    editor.setPlainText(stream.readAll());
    QCOMPARE_EQ(editor.document()->characterCount(), filesize+1);

    int matches;
    QString doc;

    timerNow(start_t);
    matches = editor.getMatchCount("ABC", {});
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 220753); /// Case insensitive 220753

    /// Count again with same search term
    timerNow(start_t);
    matches = editor.getMatchCount("ABC", {});
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 220753); /// Case insensitive 220753
    QCOMPARE_LE(timerDiffMs(start_t, end_t), 2); /// Immediate results when nothing has changed

    timerNow(start_t);
    matches = editor.getMatchCount("ABD", {});
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 0);

    timerNow(start_t);
    matches = editor.getMatchCount("bcd", {});
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 220753);
}

void Test_Search::getMatchCount_editDoc()
{
    auto filesize = filesize10MB;
    QString fileName = "testdata/10MB.txt";
    {
        QFile file(fileName);
        QVERIFY(file.open(QFile::ReadOnly));
    }

    TextStream stream(fileName);
    Editor editor;
    editor.setPlainText(stream.readAll());
    QCOMPARE_EQ(editor.document()->characterCount(), filesize+1);

    int matches;

    timerNow(start_t);
    matches = editor.getMatchCount("ABC", {});
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 220753); /// Case insensitive 220753

    editor.appendPlainText("ABC");
    timerNow(start_t);
    matches = editor.getMatchCount("ABC", {});
    timerNow(end_t);
    printTime("getMatchCount DONE", start_t, end_t);
    QCOMPARE(matches, 220754);
}


QTEST_MAIN(Test_Search)
#include "test_search.moc"

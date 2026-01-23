#include <QTest>
#include "utils/namefilterlist.hpp"


class Test_NameFilterList: public QObject
{
    Q_OBJECT

private slots:

    void initTestCase()
    {
        qDebug("initTestCase");
    }

    void getFilter();
    void getSuffix();

    void cleanupTestCase()
    {
        qDebug("cleanupTestCase");
    }
};

void Test_NameFilterList::getFilter()
{
    NameFilterList list;
    QString suffix, filter;

    suffix = "txt";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "Text files (*.txt)");
    suffix = ".txt";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "Text files (*.txt)");
    suffix = "*.txt";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "Text files (*.txt)");
    suffix = "*txt";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "Text files (*.txt)");
    suffix = "*";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "All files (*)");
    suffix = "h";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "C/C++ files (*.cpp *.hpp *.c *.h)");
    suffix = ".c";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "C/C++ files (*.cpp *.hpp *.c *.h)");
    suffix = "cpp";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "C/C++ files (*.cpp *.hpp *.c *.h)");
    suffix = "tx";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "");
    suffix = "xt";
    filter = list.getFilter(suffix, "All files (*)");
    QCOMPARE(filter, "All files (*)");
    filter = list.getFilter("", list.first());
    QCOMPARE(filter, "All files (*)");

    list = {
        {
            "NameThatContains txt (*.nottxt)",
            "Text files (*.txt)",
            "All files (*)",
            "All files (*.*)",
        }
    };
    suffix = "txt";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "Text files (*.txt)");
    suffix = "*.*";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "All files (*.*)");
    suffix = "*";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "All files (*)");

    list = {
        {
        "NameThatContains txt (*.nottxt)",
        "Text files (*.txt)",
        "All files (*.*)",
        "All files (*)",
        }
    };
    suffix = "*.*";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "All files (*.*)");
    suffix = "*";
    filter = list.getFilter(suffix);
    QCOMPARE(filter, "All files (*)");

}

void Test_NameFilterList::getSuffix()
{
    NameFilterList list;
    QString suffix, filter;

    filter = "Text files (*.txt)";
    suffix = NameFilterList::getSuffix(filter);
    QCOMPARE(suffix, "txt");

    filter = "C/C++ files (*.cpp *.hpp *.c *.h)";
    suffix = NameFilterList::getSuffix(filter);
    QCOMPARE(suffix, "cpp");

    filter = "C/C++ files (*.cpp *.hpp *.c *.h)";
    suffix = NameFilterList::getSuffix(filter, 1);
    QCOMPARE(suffix, "hpp");
    suffix = NameFilterList::getSuffix(filter, 3);
    QCOMPARE(suffix, "h");
    suffix = NameFilterList::getSuffix(filter, 4, "fb");
    QCOMPARE(suffix, "fb");

    filter = "All files (*)";
    suffix = NameFilterList::getSuffix(filter);
    QCOMPARE(suffix, "");
    suffix = NameFilterList::getSuffix(filter, 0, "fb");
    QCOMPARE(suffix, ""); /// * gives empty even if fallback is set
    filter = "All files (*.*)";
    suffix = NameFilterList::getSuffix(filter);
    QCOMPARE(suffix, "");
}

QTEST_APPLESS_MAIN(Test_NameFilterList)
#include "test_namefilterlist.moc"

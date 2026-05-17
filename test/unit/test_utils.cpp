#include <QTest>
#include "utils/utils.hpp"


class Test_Utils: public QObject
{
    Q_OBJECT

private slots:

    /// Called once before all tests
    void initTestCase()
    {
    }
    /// Called before each test function
    void init()
    {
    }
    /// Called in the end of each test function (before pass)
    void cleanup()
    {
    }
    /// Called once after all tests
    void cleanupTestCase()
    {
    }

    void mathUtils();

};

void Test_Utils::mathUtils()
{
    QCOMPARE(Utils::roundToHalf(1.25), 1.5);
    QCOMPARE(Utils::roundToHalf(1.24), 1.0);
    QCOMPARE(Utils::roundToHalf(1.7), 1.5);
    QCOMPARE(Utils::roundToHalf(1.76), 2.0);
    QCOMPARE(Utils::roundToHalf(-1.76), -2.0);
    QCOMPARE(Utils::roundToHalf(-1.74f), -1.5f);
//    QCOMPARE(Utils::roundToHalf(174), 174); /// Must not compile if C++20
}


QTEST_MAIN(Test_Utils)
#include "test_utils.moc"

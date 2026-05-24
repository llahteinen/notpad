#include "testutils.hpp"
#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDebug>


void TestUtils::generateAsciiFile(const quint64& size, const QString &name)
{
    QFile fw;
    fw.setFileName(name);
    const bool open = fw.open(QFile::WriteOnly | QFile::Text);
    Q_ASSERT(open);
    qDebug() << QFileInfo(fw).absoluteFilePath();

    if(open)
    {
        QTextStream ts(&fw);
        for(quint64 i = 0; i < size; ++i)
        {
            /// !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
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
    else
    {
        qWarning() << "File not open" << name;
    }
}

void TestUtils::timerNow(std::chrono::high_resolution_clock::time_point& t)
{
    t = std::chrono::high_resolution_clock::now();
}

int TestUtils::timerDiffMs(std::chrono::high_resolution_clock::time_point& start, std::chrono::high_resolution_clock::time_point& end)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

void TestUtils::printTime(const QString& text, std::chrono::high_resolution_clock::time_point& start, std::chrono::high_resolution_clock::time_point& end)
{
    qInfo() << text << "in" << timerDiffMs(start, end) << "ms";
}

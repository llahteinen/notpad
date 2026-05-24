#ifndef TESTUTILS_HPP
#define TESTUTILS_HPP

#include <QtTypes>
#include <chrono>

class QString;
using namespace std::chrono_literals;


namespace TestUtils
{
    /// !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~
    void generateAsciiFile(const quint64& size, const QString& name);

    void timerNow(std::chrono::high_resolution_clock::time_point& t);
    int timerDiffMs(std::chrono::high_resolution_clock::time_point& start, std::chrono::high_resolution_clock::time_point& end);
    void printTime(const QString& text, std::chrono::high_resolution_clock::time_point& start, std::chrono::high_resolution_clock::time_point& end);
};


#endif // TESTUTILS_HPP

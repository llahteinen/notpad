#ifndef UTILS_HPP
#define UTILS_HPP

#include <QStringList>

class QMimeData;


namespace Utils
{

    bool hasValidFiles(const QMimeData* mimeData);

    QStringList toFilelist(const QMimeData* mimeData);

    template<std::floating_point T>
    T roundToHalf(T input)
    {
        return std::round(input * T(2)) / T(2);
    }
};


#endif // UTILS_HPP

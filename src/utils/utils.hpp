#ifndef UTILS_HPP
#define UTILS_HPP

#include <QStringList>

class QMimeData;


namespace Utils
{

    bool hasValidFiles(const QMimeData* mimeData);

    QStringList toFilelist(const QMimeData* mimeData);

};


#endif // UTILS_HPP

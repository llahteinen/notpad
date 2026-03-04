#include "utils.hpp"
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>


namespace Utils
{

bool hasValidFiles(const QMimeData* mimeData)
{
    bool all_ok = mimeData->hasUrls();
    if(all_ok)
    {
        const auto urls = mimeData->urls();
        all_ok = !mimeData->urls().empty();
        for(auto& file_url : urls)
        {
            /// e.g. file:///K:/Code/Qt/Test/IMG_20210228_202231.jpg
            const QFileInfo file_info{file_url.toLocalFile()};
            all_ok &= file_info.isFile() && !file_info.isDir();
        }
        if(!all_ok)
        {
            qInfo() << "Some of the items were not files";
        }
    }
    return all_ok;
}

QStringList toFilelist(const QMimeData* mimeData)
{
    QStringList file_names;
    if(mimeData->hasUrls())
    {
        const auto urls = mimeData->urls();
        bool all_ok = !mimeData->urls().empty();
        qDebug() << urls;
        for(auto& file_url : urls)
        {
            /// e.g. file:///K:/Code/Qt/Test/IMG_20210228_202231.jpg
            const QFileInfo file_info{file_url.toLocalFile()};
            if(file_info.isFile() && !file_info.isDir())
            {
                file_names.append(file_info.filePath());
            }
            else all_ok = false;
        }
        if(!all_ok || file_names.empty())
        {
            qInfo() << "Some of the items were not files";
            file_names.clear();
        }
    }
    return file_names;
}

}

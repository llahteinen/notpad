#include "file.hpp"
#include "settings.hpp"
#include <QFileInfo>
#include <QDebug>


bool File::saveFile(QStringView text, QFile* const file)
{
    qDebug() << "File::saveFile";
    Q_ASSERT(file);
    if(file->exists())
    {
        qDebug() << "About to overwrite" << file->fileName();
    }
    else
    {
        qDebug() << "Writing new file" << file->fileName();
    }
    const QFileInfo fileInfo(*file);
    SETTINGS.currentDir = fileInfo.dir();

    if(!file->open(QFile::WriteOnly | QFile::Text))
    {
        qWarning() << "Failed to open file:" << file->errorString();
//        statusBar()->showMessage(tr("Cannot open file for writing %1:\n%2.").arg(QDir::toNativeSeparators(file->fileName()), file->errorString()));
        return false;
    }

    QTextStream fstream{file};
    fstream.setEncoding(QStringConverter::Utf8);
    fstream << text;
    if(fstream.status() != QTextStream::Ok)
    {
        qWarning() << "Failed to write to file:" << fstream.status();
//        statusBar()->showMessage(tr("File write failed %1:\n%2.").arg(fstream.status()));
        file->close();
        return false;
    }

    fstream.flush(); // Ensure data is written to the file
    if(file->error() != QFile::NoError)
    {
        qWarning() << "File error after writing:" << file->errorString();
//        statusBar()->showMessage(tr("File write failed %1:\n%2.").arg(file->errorString()));
        file->close();
        return false;
    }

    file->close(); /// Free the file resource for use by other processes
//    statusBar()->showMessage(tr("File saved: %1").arg(QDir::toNativeSeparators(file->fileName())));
    return true;
}

bool File::saveFile(std::unique_ptr<QFile>& file_p, QStringView text, const QString& fileName)
{
    qDebug() << "File::saveFile";
    file_p = std::make_unique<QFile>(fileName);
    Q_ASSERT(file_p);
    return saveFile(text, file_p.get());
}

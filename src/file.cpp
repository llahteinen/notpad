#include "file.hpp"
#include "settings.hpp"
#include <QFileInfo>
#include <QDebug>


File::Status File::saveFile(QStringView text, QFile* const file)
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
        return File::Status::FAIL_OPEN_WRITE;
    }

    QTextStream fstream{file};
    fstream.setEncoding(QStringConverter::Utf8);
    fstream << text;
    if(fstream.status() != QTextStream::Ok)
    {
        qWarning() << "Failed to write to file:" << fstream.status();
        file->close();
        return File::Status::FAIL_WRITE;
    }

    fstream.flush(); // Ensure data is written to the file
    if(file->error() != QFile::NoError)
    {
        qWarning() << "File error after writing:" << file->errorString();
        file->close();
        return File::Status::FAIL_WRITE;
    }

    file->close(); /// Free the file resource for use by other processes
    return File::Status::SUCCESS_WRITE;
}

File::Status File::saveFile(std::unique_ptr<QFile>& file_p, QStringView text, const QString& fileName)
{
    qDebug() << "File::saveFile";
    file_p = std::make_unique<QFile>(fileName);
    Q_ASSERT(file_p);
    return saveFile(text, file_p.get());
}

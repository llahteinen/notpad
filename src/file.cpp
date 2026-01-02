#include "file.hpp"
#include "settings.hpp"
#include <QFileInfo>
#include <QDebug>


File::Status File::saveFile(QStringView text, QFile* const file)
{
    qDebug() << "File::saveFile";
    Q_ASSERT(file);

    Status status{Status::UNKNOWN, file->fileName()};

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
        status.code = Status::FAIL_OPEN_WRITE;
        status.errorString = file->errorString();
        return status;
    }

    QTextStream fstream{file};
    fstream.setEncoding(QStringConverter::Utf8);
    fstream << text;
    if(fstream.status() != QTextStream::Ok)
    {
        qWarning() << "Failed to write to file:" << fstream.status();
        file->close();
        status.code = Status::FAIL_WRITE;
        status.errorString = file->errorString();
        return status;
    }

    fstream.flush(); // Ensure data is written to the file
    if(file->error() != QFile::NoError)
    {
        qWarning() << "File error after writing:" << file->errorString();
        file->close();
        status.code = Status::FAIL_WRITE;
        status.errorString = file->errorString();
        return status;
    }

    file->close(); /// Free the file resource for use by other processes
    status.code = Status::SUCCESS_WRITE;
    return status;
}

File::Status File::saveFile(std::unique_ptr<QFile>& file_p, QStringView text, const QString& fileName)
{
    qDebug() << "File::saveFile";
    file_p = std::make_unique<QFile>(fileName);
    Q_ASSERT(file_p);
    return saveFile(text, file_p.get());
}

File::Status File::openFile(std::unique_ptr<QFile>& file, const QString& fileName)
{
    qDebug() << "File::openFile" << fileName;

    Status status{Status::UNKNOWN, fileName};

    file = std::make_unique<QFile>(fileName);
    if(!file->exists())
    {
        qWarning() << "Does not exist";
        status.code = Status::FAIL_OPEN_NOTFOUND;
        status.errorString = "File not found";
        return status;
    }
    Q_ASSERT(file);

    status.fileName = file->fileName();

    QFileInfo fileInfo(*file);
    SETTINGS.currentDir = fileInfo.dir();
//    SETTINGS.currentDir = fileInfo.absoluteDir();

    qDebug() << QString("File opened: %1").arg(QDir::toNativeSeparators(fileName));

    if(!file->open(QFile::ReadOnly | QFile::Text))
    {
        qWarning() << "Can't open";
        status.code = Status::FAIL_OPEN_READ;
        status.errorString = file->errorString();
        return status;
    }

    status.code = Status::SUCCESS_READ;
    return status;
}

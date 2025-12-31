#ifndef FILE_HPP
#define FILE_HPP

#include <QFile>
#include <QString>


namespace File
{
    enum class Status
    {
        UNKNOWN = 0,
        CANCELED,
        FAIL_OPEN_WRITE,
        FAIL_WRITE,
        FAIL_WRITE_UNKNOWN,
        SUCCESS_WRITE,
    };

    /// \brief Save to a file using QFile pointer.
    /// \param text Text to be saved to the file
    /// \param file_p Pointer to an existing QFile instance
    /// \return Success
    Status saveFile(QStringView text, QFile* const file);

    /// \brief Save to a file using file name. Outputs file_p pointer of the newly created QFile.
    /// \param file_p[out]
    /// \param text Text to be saved to the file
    /// \param fileName
    /// \return Success
    Status saveFile(std::unique_ptr<QFile>& file_p, QStringView text, const QString& fileName);
};

#endif // FILE_HPP

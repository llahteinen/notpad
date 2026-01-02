#ifndef FILE_HPP
#define FILE_HPP

#include <QString>

class QFile;

namespace File
{
    struct Status
    {
        enum ECode
        {
            UNKNOWN = 0,
            CANCELED,
            FAIL_OPEN_WRITE,
            FAIL_WRITE,
            FAIL_WRITE_UNKNOWN,
            FAIL_OPEN_NOTFOUND,
            FAIL_OPEN_READ,
            SUCCESS_WRITE,
            SUCCESS_READ,
        };

        Status() :
            code{ECode::UNKNOWN},
            fileName{},
            errorString{}
        {}
        Status(ECode ecode) :
            code{ecode},
            fileName{},
            errorString{}
        {}
        Status(ECode ecode, QString fileName, QString errorString = "") :
            code{ecode},
            fileName{fileName},
            errorString{errorString}
        {}

        bool operator==(const ECode& ecode) const { return ecode == this->code; }
        bool operator!=(const ECode& ecode) const { return ecode != this->code; }
        operator ECode() const { return this->code; } /// Allow implicit cast to ECode enum
        Status& operator=(ECode) = delete; /// Prevent assignment directly from the ECode enum

        ECode code;
        QString fileName;
        QString errorString;
    };

    /// \brief Save to a file using QFile reference.
    /// \param text Text to be saved to the file
    /// \param file File to be saved to. Must have filename already set
    /// \return Success status object
    Status saveFile(QStringView text, QFile& file);

    /// \brief Open a file for reading, but don't read anything.
    /// \param file File to be opened
    /// \param fileName
    /// \return Success status object
    Status openFile(QFile& file, const QString& fileName);
};

#endif // FILE_HPP

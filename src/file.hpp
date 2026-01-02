#ifndef FILE_HPP
#define FILE_HPP

#include <QFile>
#include <QString>


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

    Status openFile(std::unique_ptr<QFile>& file_p, const QString& fileName);
};

#endif // FILE_HPP

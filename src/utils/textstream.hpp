#ifndef TEXTSTREAM_H
#define TEXTSTREAM_H

#include <QTextStream>
#include <QObject>
#include <QFile>


class TextStream : public QObject, public QTextStream
{
    Q_OBJECT
public:
    enum class EncodingError
    {
        UNAVAILABLE = -1,
        FALSE = 0,
        TRUE = 1,
    };

    explicit TextStream(QFileDevice* device);
    explicit TextStream(const QString& fileName); /// Use with readChunks

    QString readAll();

    void setAutoDetectBom(bool enabled);
    bool hasBom() const;

    void setValidateUtf(bool enabled);
    EncodingError hasUtfError() const;

    void setValidateLatin(bool enabled);
    EncodingError hasLatinError() const;

    struct MetaData
    {
        bool fileError{false};
        QStringConverter::Encoding encoding{QStringConverter::Encoding::Utf8};
        bool hasBom{false};
        EncodingError hasUtfError{EncodingError::UNAVAILABLE};
        EncodingError hasLatinError{EncodingError::UNAVAILABLE};
        bool done{false};
    };

public slots:
    /// \brief readChunks Opens a file with m_file, reads it and emits dataAvailable periodically until the whole file is read
    void readChunks();

signals:
    void dataAvailable(const QString& dataChunk, const TextStream::MetaData& meta);

private:
    void doValidations();

    bool m_autoDetectBom{false};
    bool m_validateUtf{false};
    bool m_validateLatin{false};
    bool m_hasBom{false};
    EncodingError m_hasUtfError{EncodingError::UNAVAILABLE};
    EncodingError m_hasLatinError{EncodingError::UNAVAILABLE};
    QFile m_file; /// Own instance here for thread safety
};

#endif // TEXTSTREAM_H

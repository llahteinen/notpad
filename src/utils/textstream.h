#ifndef TEXTSTREAM_H
#define TEXTSTREAM_H

#include <QTextStream>


class TextStream : public QTextStream
{
public:
    enum class EncodingError
    {
        UNAVAILABLE = -1,
        FALSE = 0,
        TRUE = 1,
    };

    explicit TextStream(QIODevice* device);

    QString readAll();

    void setAutoDetectBom(bool enabled);
    bool hasBom() const;

    void setValidateUtf(bool enabled);
    EncodingError hasUtfError() const;

    void setValidateLatin(bool enabled);
    EncodingError hasLatinError() const;

private:
    bool m_autoDetectBom{false};
    bool m_validateUtf{false};
    bool m_validateLatin{false};
    bool m_hasBom{false};
    EncodingError m_hasUtfError{EncodingError::UNAVAILABLE};
    EncodingError m_hasLatinError{EncodingError::UNAVAILABLE};
};

#endif // TEXTSTREAM_H

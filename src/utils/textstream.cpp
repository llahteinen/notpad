#include "textstream.h"
#include <QIODevice>
#include <QDebug>


TextStream::TextStream(QIODevice* device) : QTextStream(device) {}

QString TextStream::readAll()
{
    /// Qt encoding detection works by simply reading a BOM. If BOM is not found, encoding is not changed.
    /// So we can use that to infer the possible BOM in the file.
    /// NOTE Depends on QStringConverter::encodingForData implementation. If implementation changes, this might break.
    /// Latin1 validation needs BOM information
    if(m_autoDetectBom || m_validateLatin)
    {
        m_autoDetectBom = false;
        const auto bav = device()->bytesAvailable();
        const auto maybe_bom = device()->peek(qMin(4, bav));
        const auto e = QStringConverter::encodingForData(maybe_bom);
        m_hasBom = e.has_value();
    }

    /// Don't validate utf if encoding is set to something else than utf
    if(m_validateUtf && QString(QStringConverter::nameForEncoding(encoding())).contains("utf", Qt::CaseInsensitive))
    {
        static constexpr qint64 max = 4096;
        const auto bav = device()->bytesAvailable();
        const auto check_bytes = device()->peek(qMin(bav, max));

        /// TODO put this in a loop and break on the first error
        /// "The decoder remembers any state that is required between calls, so converting data received in chunks, for example,
        /// when receiving it over a network, is just as easy, by calling the decoder whenever new data is available:"
        QStringDecoder decoder(QStringDecoder::Utf8);
        const QString res = decoder(check_bytes); /// Need to read to QString first to get hasError data
        if(decoder.hasError())
        {
            m_hasUtfError = EncodingError::TRUE;
            setEncoding(QStringConverter::Encoding::Latin1); /// Or should use Encoding::System?
        }
        else
        {
            m_hasUtfError = EncodingError::FALSE;
        }
    }

    /// If encoding is Latin1, file should not contain BOM
    if(m_validateLatin && encoding() == QStringConverter::Encoding::Latin1)
    {
        if(m_hasBom)
        {
            m_hasLatinError = EncodingError::TRUE;
            qWarning() << "Encoding is Latin1 but file has BOM";
        }
    }

    return QTextStream::readAll();
}

void TextStream::setAutoDetectBom(bool enabled)
{
    m_autoDetectBom = enabled;
}

bool TextStream::hasBom() const
{
    return m_hasBom;
}

void TextStream::setValidateUtf(bool enabled)
{
    m_validateUtf = enabled;
}

TextStream::EncodingError TextStream::hasUtfError() const
{
    return m_hasUtfError;
}

void TextStream::setValidateLatin(bool enabled)
{
    m_validateLatin = enabled;
}

TextStream::EncodingError TextStream::hasLatinError() const
{
    return m_hasLatinError;
}

#if 0
/// From qstringconverter.cpp
std::optional<QStringConverter::Encoding>
TextStream::encodingForData(QByteArrayView data, char16_t expectedFirstCharacter) noexcept
{
    // someone set us up the BOM?
    qsizetype arraySize = data.size();
    if (arraySize > 3) {
        char32_t uc = qFromUnaligned<char32_t>(data.data());
        if (uc == qToBigEndian(char32_t(QChar::ByteOrderMark)))
            return QStringConverter::Utf32BE;
        if (uc == qToLittleEndian(char32_t(QChar::ByteOrderMark)))
            return QStringConverter::Utf32LE;
        if (expectedFirstCharacter) {
            // catch also anything starting with the expected character
            if (qToLittleEndian(uc) == expectedFirstCharacter)
                return QStringConverter::Utf32LE;
            else if (qToBigEndian(uc) == expectedFirstCharacter)
                return QStringConverter::Utf32BE;
        }
    }
    if (arraySize > 2) {
        if (memcmp(data.data(), utf8bom, sizeof(utf8bom)) == 0)
            return QStringConverter::Utf8;
    }
    if (arraySize > 1) {
        char16_t uc = qFromUnaligned<char16_t>(data.data());
        if (uc == qToBigEndian(char16_t(QChar::ByteOrderMark)))
            return QStringConverter::Utf16BE;
        if (uc == qToLittleEndian(char16_t(QChar::ByteOrderMark)))
            return QStringConverter::Utf16LE;
        if (expectedFirstCharacter) {
            // catch also anything starting with the expected character
            if (qToLittleEndian(uc) == expectedFirstCharacter)
                return QStringConverter::Utf16LE;
            else if (qToBigEndian(uc) == expectedFirstCharacter)
                return QStringConverter::Utf16BE;
        }
    }
    return std::nullopt;
}
#endif

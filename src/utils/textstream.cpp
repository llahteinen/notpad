#include "textstream.hpp"
#include "file.hpp"
#include <QIODevice>
#include <QFileInfo>
#include <QDebug>


TextStream::TextStream(QFileDevice* device) : TextStream(device->fileName()) {}

TextStream::TextStream(const QString& fileName) : QTextStream(), m_file{fileName, this}
{
    qRegisterMetaType<MetaData>();
    /// Crash if QTextStream constructor is called with m_file
    setDevice(&m_file);
}

/// NOTE device has to be set before calling this
void TextStream::doValidations()
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
    /// By default we have set utf8 and detecting utf16 comes later
    /// So basically this validateUtf can't work with utf16
    /// It causes "Encoding is Latin1 but file has BOM"
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
    /// If file does NOT contain BOM, then it's not possible to validate
    if(m_validateLatin && encoding() == QStringConverter::Encoding::Latin1)
    {
        if(m_hasBom)
        {
            m_hasLatinError = EncodingError::TRUE;
            qWarning() << "Encoding is Latin1 but file has BOM";
        }
        else
        {
            m_hasLatinError = EncodingError::UNAVAILABLE;
        }
    }
}

/// NOTE device has to be set before calling this
/// because setDevice() here will reset parameters such as autodetectUnicode
QString TextStream::readAll()
{
    if(File::openFile(m_file, m_file.fileName()) != File::Status::SUCCESS_READ)
    {
        qDebug() << "readAll openFile not successful";
        return {};
    }

    doValidations();

    /// QTextStream::read(100000) seems to be more than 10x faster than QTextStream::readAll()
    /// They both use a QTEXTSTREAM_BUFFERSIZE = 16384 for reading the raw data
    /// They use a QString readBuffer for the parsed data
    /// readBuffer += toUtf16 increases QString readBuffer size by QTEXTSTREAM_BUFFERSIZE leading to realloc
    /// With readAll() readBuffer keeps growing until it can hold the whole file
    /// It is unclear whether consecutive read() calls makes the readBuffer to shrink and inflate or if it stays the max of previous call
    /// Maybe readAll is slower than read just because the realloc becomes more expensive as the string size grows
    QString ret;
//    const auto bav = device()->bytesAvailable();
//    ret.reserve(bav); /// Might speed up a tiny bit. But this is already fast. And would need to add some logic for different utfs
    static constexpr qint64 maxChunkSize = 100000;
    while(!QTextStream::atEnd())
    {
        ret += QTextStream::read(maxChunkSize);
    }
    device()->close();
    return ret;
}

/// NOTE device has to be set before calling this
/// because setDevice() here will reset parameters such as autodetectUnicode
void TextStream::readChunks()
{
    MetaData meta;

    if(File::openFile(m_file, m_file.fileName()) != File::Status::SUCCESS_READ)
    {
        qDebug() << "readChunks openFile not successful";
        meta.fileError = true;
        meta.done = true;
        emit dataAvailable({}, meta);
        return;
    }
    qDebug() << "bytesAvailable" << device()->bytesAvailable();

    doValidations();

    /// 20000 is very roughly a full screen amount of text on a normal font
    static constexpr qint64 maxChunkSize = 100000;
    QString data;
//    data.reserve(maxChunkSize); /// Does not seem to affect speed in meaningful way
    while(!QTextStream::atEnd())
    {
        /// Seems like this is extremely faster than QTextStream::readAll()
        /// It also reallocates readBuffer every read of QTEXTSTREAM_BUFFERSIZE
        /// Then it seems to resize it down again when returning, calls readBuffer.remove, which shouldn't free the memory though
        /// Both use roughly same amount of ram
        data = QTextStream::read(maxChunkSize);
        meta.encoding = encoding(); /// encoding might be updated in QTextStream::read()
        meta.hasBom = m_hasBom;
        meta.hasUtfError = m_hasUtfError;
        meta.hasLatinError = m_hasLatinError;
        meta.done = QTextStream::atEnd();
        emit dataAvailable(data, meta);
//        QThread::msleep(5); /// This allows UI to update, not overwhelming event queue
    }

    /// Is this called in case of exceptions?
    device()->close();
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

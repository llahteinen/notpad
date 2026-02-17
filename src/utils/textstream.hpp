#ifndef TEXTSTREAM_H
#define TEXTSTREAM_H

#include <QTextStream>
#include <QObject>
#include <QFile>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>


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

    /// \brief metaQueue Contains metadata for the data produced by readChunks()
    QQueue<MetaData>& metaQueue() { return m_metaQueue; };
    /// \brief dataQueue Contains the data produced by readChunks()
    QQueue<QString>& dataQueue() { return m_dataQueue; };
    /// \brief queueMutex Protects metaQueue and dataQueue. Lock this before accessing them.
    QMutex* queueMutex() { return &m_dataMutex; };

    void setBatchSize(int batch_size)
    {
        batch_size = qMin(maxBatchSize, batch_size);
        m_batchSize = qMax(1, batch_size);
    };
    int batchSize() const { return m_batchSize; };

    void setThrottlingEnabled(bool enabled) { m_throttlingEnabled = enabled; };

    /// \brief decrementThrottle
    /// locks queueMutex inside, so queueMutex must be unlocked before calling
    void decrementThrottle();
    /// \brief decrementThrottle
    /// \param lock Lock object for queueMutex. Locked state will be same after this function like it was before calling this.
    void decrementThrottle(QMutexLocker<QMutex>& lock);

public slots:
    /// \brief readChunks Opens a file with m_file, reads it and emits dataQueued periodically until the whole file is read
    void readChunks();

    void quit();

signals:
    void dataQueued();  /// This is emitted when readChunks has worked one chunk of data and put it into m_dataQueue and m_metaQueue

private:
    void doValidations();

    std::atomic_bool m_quitCalled{false};
    QWaitCondition m_dataWait{};
    QMutex m_dataMutex{};
    QQueue<MetaData> m_metaQueue{};
    QQueue<QString> m_dataQueue{};
    std::atomic_bool m_throttlingEnabled{true};
    std::atomic_int m_updateThrottle{0};
    std::atomic_int m_batchSize{1};
    /// 20000 is very roughly a full screen amount of text on a normal font
    static constexpr qint64 maxChunkSize = 100000;
    /// Read data amount per one iteration is m_batchSize * maxChunkSize
    /// maxBatchSize is maximum limit for m_batchSize
    static constexpr int maxBatchSize = 100;

    bool m_autoDetectBom{false};
    bool m_validateUtf{false};
    bool m_validateLatin{false};
    bool m_hasBom{false};
    EncodingError m_hasUtfError{EncodingError::UNAVAILABLE};
    EncodingError m_hasLatinError{EncodingError::UNAVAILABLE};
    QFile m_file; /// Own instance here for thread safety
};

#endif // TEXTSTREAM_H

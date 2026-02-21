#include "editor.hpp"
#include "utils/regex.hpp"
#include "settings.hpp"
#include <QFileDialog>
#include <QFileInfo>
#include <QRegularExpression>
#include <QThread>
#include <QElapsedTimer>


Editor::Editor(QWidget *parent)
    : Editor(QString{}, {nullptr}, parent)
{}

Editor::Editor(const QString& text, std::unique_ptr<QFile> file_p, QWidget *parent)
    : QPlainTextEdit(text, parent)
    , highLighter{new Highlighter(this->document())}
    , m_name{SETTINGS.defaultDocName}
    , m_textStream{nullptr}
    , m_file{std::move(file_p)}
    , m_encoding{QStringConverter::Utf8}
    , m_hasBom{false}
    , m_format{}
    , m_search{}
{
    if(m_file)
    {
        m_name = QFileInfo(*m_file).fileName();
    }

    /// QSyntaxHighlighter::rehighlight() does emit contentsChanged for some reason but not contentsChange
    /// We want contentsChange because only real edits should trigger it
    connect(document(), &QTextDocument::contentsChange, this, &Editor::onContentsChange);
}

Editor::Editor(TextStream* stream, std::unique_ptr<QFile> file_p, QWidget *parent)
    : QPlainTextEdit(parent)
    , highLighter{new Highlighter(this->document())}
    , m_name{SETTINGS.defaultDocName}
    , m_textStream{stream}
    , m_file{std::move(file_p)}
    , m_encoding{QStringConverter::Utf8}
    , m_hasBom{false}
    , m_format{}
    , m_search{}
{
    if(m_file)
    {
        m_name = QFileInfo(*m_file).fileName();
    }

    /// QSyntaxHighlighter::rehighlight() does emit contentsChanged for some reason but not contentsChange
    /// We want contentsChange because only real edits should trigger it
    connect(document(), &QTextDocument::contentsChange, this, &Editor::onContentsChange);

    connect(m_textStream, &TextStream::dataQueued, this, &Editor::onDataQueued,
            Qt::QueuedConnection); /// QueuedConnection just to be sure (dataAvailable and onDataAvailable are supposed to run in different threads)
}

Editor::~Editor()
{
    m_textStream->quit();
    bool finished = m_textStream->thread()->wait(1000);
    qDebug() << "textStream thread finished:" << finished;
    if(!finished)
    {
        qWarning() << m_name << "worker thread did not exit cleanly";
        m_textStream->thread()->terminate();
    }
    /// m_textStream will be deleted by its thread's finished signal
}

/// static
Editor* Editor::createEditor(File::Status& o_status, const QString& fileName, QWidget* parent)
{
    auto file_p = std::make_unique<QFile>();
    o_status = File::openFile(*file_p, fileName);
    if(o_status != File::Status::SUCCESS_READ)
    {
        file_p->close();
        return nullptr;
    }
    file_p->close(); /// This was just for initial check. Create and open a new QFile in the other thread since QFile is not marked thread safe.

//    /// Read synchronously in some circumstance (small files)?
//    TextStream fileStream(file_p.get());
//    fileStream.setEncoding(QStringConverter::Encoding::Utf8);
//    fileStream.setAutoDetectUnicode(true);
//    fileStream.setAutoDetectBom(true);
//    fileStream.setValidateUtf(true); /// Does not really work for utf16
////    fileStream.setValidateUtf(false); /// Does not detect latin and defaults to utf8
//    fileStream.setValidateLatin(true);
////    Editor* editor = new Editor(fileStream.readAll(), std::move(file_p), parent);
//    /// For testing:
//    /// Appending the string into the editor is very slow
//    /// Takes a lot of RAM
//    const QString str = fileStream.readAll();
//    Editor* editor = new Editor(str, std::move(file_p), parent);
//    editor->m_encoding = fileStream.encoding();
//    editor->m_hasBom = fileStream.hasBom();
//    qDebug() << "encoding" << QStringConverter::nameForEncoding(editor->m_encoding);
//    /// file_p is nullptr now
//    fileStream.device()->close();

    /// Asynchronous, transfer the data via signals
    TextStream* tStream = new TextStream(fileName);
    tStream->setEncoding(QStringConverter::Encoding::Utf8);
    tStream->setAutoDetectUnicode(true);
    tStream->setAutoDetectBom(true);
    tStream->setValidateUtf(true); /// Does not really work for utf16
    tStream->setValidateLatin(true);

    QThread* thread = new QThread();
    /// Slots of tStream will be executed in the new thread's event loop
    /// Must not call any tStream methods from the original thread
    /// Do not give tStream a parent that lives in the main thread (Editor) "All QObjects must live in the same thread as their parent"
    /// moveToThread is called before connect in Qt's example
    if(const bool threadMoved = tStream->moveToThread(thread) == false)
    {
        qWarning() << "moveToThread failed";
        Q_ASSERT(threadMoved);
    }
    /// Run readChunks as soon as the thread starts
    connect(thread, &QThread::started, tStream, &TextStream::readChunks); /// AutoConnection?
    /// Stop the thread's event loop when tStream is deleted
    connect(tStream, &TextStream::destroyed, thread, &QThread::quit);
    /// Delete tStream after its thread has finished
    connect(thread, &QThread::finished, tStream, &QObject::deleteLater);
    /// Delete the thread after it has finished
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    Editor* editor = new Editor(tStream, std::move(file_p), parent);

    /// QThread enters its own event loop here which executes until exit is called (or quit)
    editor->m_start_t = std::chrono::high_resolution_clock::now(); /// DEBUG
    thread->start();

    return editor;
}

void Editor::onDataQueued()
{
    QMutexLocker lock(m_textStream->queueMutex());

    Q_ASSERT(!m_textStream->metaQueue().isEmpty());
    Q_ASSERT(!m_textStream->dataQueue().isEmpty());

    QElapsedTimer timer;
    timer.start();

    const auto [meta, dataChunk] = m_textStream->dequeue();

    /// This should never happen, checks should've been done for file many times already
    if(Q_UNLIKELY(meta.fileError))
    {
        qWarning() << "File error" << m_name;
        return;
    }

    lock.unlock();

    QTextCursor cursor(document());

    if(Q_UNLIKELY(meta.first))
    {
        m_encoding = meta.encoding;
        m_hasBom = meta.hasBom;

        setReadOnly(true);
        blockSignals(true); /// Not 100% sure, should we set the signals enabled on the last iteration?
        document()->blockSignals(true);
        setUndoRedoEnabled(false);

        /// Small optimization because QTextCursor::insertText(const QString &text) copies the format on every call
        m_format = cursor.charFormat();
    }

    cursor.movePosition(QTextCursor::End); /// Ensure we are appending to the end
    cursor.insertText(dataChunk, m_format); /// Sets modified flag to true
    document()->setModified(false);

    blockSignals(false);
    emit dataLoadingUpdate(document()->characterCount()-1); /// Blocksignals must be false

    /// Adapt the text insert amount to desired UI update rate
    /// Insert largest possible blocks because it's faster, but without exceeding frame time
    const qint64 elapsed = timer.elapsed();
//    static constexpr qint64 FRAME_TIME_TARGET = 10; /// ~100 FPS Higher update rate impacts the total time but not a lot
    static constexpr qint64 FRAME_TIME_TARGET = 17; /// ~60 FPS Feels quite smooth, though not 120Hz display smooth
//    static constexpr qint64 FRAME_TIME_TARGET = 25; /// ~40 FPS

    if(elapsed < FRAME_TIME_TARGET)
    {
        /// Too small value here slightly slows down. Too big value does not give any benefit
        m_textStream->setBatchSize(m_textStream->batchSize() + 5);
    }
    else if(elapsed > FRAME_TIME_TARGET)
    {
        m_textStream->setBatchSize(m_textStream->batchSize() - 2);
    }

    /// Notify that we are ready to receive more data in gui thread
    m_textStream->decrementThrottle(lock);

    /// This gets processed only when all the signals are processed, even if the data queue went empty way earlier
    if(Q_UNLIKELY(meta.done))
    {
        m_end_t = std::chrono::high_resolution_clock::now(); /// DEBUG
        qInfo() << "insertText DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(m_end_t-m_start_t).count();
        qDebug() << "Final batchSize" << m_textStream->batchSize();

        /// 10000 = 5,3s
        /// 20000 = 3,1s
        /// 100000 = 1,4s

//        /// Try to retrieve the user's wanted cursor position after the insert operations
//        userCursor.setPosition(userAnchor, QTextCursor::MoveAnchor);
//        userCursor.setPosition(userPosition, QTextCursor::KeepAnchor);
//        setTextCursor(userCursor);
        /// Jump the cursor to start
        /// We would like that the cursor would be at the start of the file, but the viewport scrolling would stay where it is
        /// document()->blockSignals(true) does not help
        /// Workaround: Set the cursor to the top of the current viewport location
        QTextCursor userCursor = textCursor();
        userCursor.setPosition(firstVisibleBlock().position());
        setTextCursor(userCursor);
//        setFocus(); /// Not sure about this

        document()->setModified(false);
        document()->blockSignals(false);
        blockSignals(false);
        setReadOnly(false);

        /// Settings
        setUndoRedoEnabled(true);

        emit dataLoadingFinished();
    }
}

bool Editor::saveOrSaveAs()
{
    return m_file != nullptr;
}

File::Status Editor::save()
{
    qDebug() << "Editor::save";
    Q_ASSERT(m_file != nullptr && "m_file can't be nullptr when saving");

    /// Note: toPlainText() creates a copy of all the text inside the QPlainTextEdit
    File::Status saved = File::saveFile(toPlainText(), *m_file, m_encoding, m_hasBom);

    if(saved == File::Status::SUCCESS_WRITE)
    {
        document()->setModified(false);
    }
    return saved;
}

File::Status Editor::saveAs(const QString& fileName)
{
    qDebug() << "Editor::saveAs" << fileName;
    m_file.reset();
    auto file = std::make_unique<QFile>(fileName);
    File::Status saved = File::saveFile(toPlainText(), *file, m_encoding, m_hasBom);

    if(saved == File::Status::SUCCESS_WRITE)
    {
        m_file = std::move(file);
        setName(QFileInfo(*m_file).fileName());
        document()->setModified(false);
    }
    return saved;
}

qsizetype Editor::getMatchCount(const QString& sterm, QTextDocument::FindFlags flags)
{
    qDebug() << "getMatchCount" << sterm;
    flags.setFlag(QTextDocument::FindBackward, false); /// Don't store search direction

    if(m_search.matchCount < 0 || sterm != m_search.term || flags != m_search.flags)
    {
        m_search.term = sterm;
        m_search.flags = flags;
        m_search.matchCount = countMatches(sterm, flags);
    }
    return m_search.matchCount;
}

void Editor::setName(const QString& name)
{
    if(name != m_name)
    {
        m_name = name;
        emit nameChanged(m_name);
    }
}

QString Editor::name() const
{
    return m_name;
}

const QFile* Editor::file() const
{
    return m_file.get();
}

QString Editor::encodingName() const
{
    QString name = QStringConverter::nameForEncoding(m_encoding);
    if(m_hasBom) name.append(" BOM");
    return name;
}

bool Editor::isModified() const
{
    return document()->isModified();
}

void Editor::setWordWrap(bool enabled)
{
    const auto wrap_mode = enabled ? QTextOption::WrapMode::WrapAtWordBoundaryOrAnywhere
                                   : QTextOption::WrapMode::NoWrap;
    /// NOTE: maybe with binary files could use WrapAnywhere
    setWordWrapMode(wrap_mode);
}

bool Editor::isWordWrap() const
{
    return !(wordWrapMode() == QTextOption::WrapMode::NoWrap);
}

void Editor::updateTabWidth()
{
    setTabStopDistance(SETTINGS.tabWidthChars * fontMetrics().averageCharWidth());
}

void Editor::setFont(const QFont& font)
{
    QPlainTextEdit::setFont(font);
    updateTabWidth();
}

void Editor::onContentsChange([[maybe_unused]]int position, [[maybe_unused]]int charsRemoved, [[maybe_unused]]int charsAdded)
{
//    qDebug() << "onContentsChange" << position << "" << charsRemoved << "" << charsAdded;
    invalidateSearchResults();
}

void Editor::invalidateSearchResults()
{
    qDebug() << "invalidateSearchResults";
    m_search = {};
}

qsizetype Editor::countMatches(const QString& sterm, QTextDocument::FindFlags flags)
{
    qDebug() << "countMatches";

    const auto doc = document()->toRawText();
//    const auto doc = document()->toPlainText(); /// Check later if should use this if toRawText has issues with unicode or something

    /// Regex method (this is faster than QString::count(QString))
    const auto reg = Regex::stringToRegex(sterm, flags);
    if(!reg.isValid())
    {
        return -1;
    }

    const auto matchIterator = reg.globalMatch(doc);
    if(!matchIterator.isValid())
    {
        return -1;
    }

    qsizetype count = 0;
    static constexpr auto max_val = std::numeric_limits<decltype(count)>::max();
    for([[maybe_unused]]const auto& _ : matchIterator)
    {
        if(count == max_val)
        {
            count = -2;
            qInfo() << "Too many search results: >" << max_val;
            break;
        }
        ++count;
    }

    qDebug() << "resultCount" << count;
    return count;
}

QList<QTextCursor> Editor::findAll(const QString& sterm, QTextDocument::FindFlags flags)
{
    qDebug() << "findAll";
    flags.setFlag(QTextDocument::FindBackward, false); /// Always search forward, but respect the other flags
    QList<QTextCursor> results;
    QTextCursor result;
    int pos = 0;

    do
    {
        result = document()->find(sterm, pos, flags);
        if(!result.isNull())
        {
            pos = result.position(); /// must be position(), not position() + 1
            results.append(result);
        }
    } while(!result.isNull());

    return results;
}

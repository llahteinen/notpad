#include "editor.hpp"
#include "settings.hpp"
#include <QFileDialog>
#include <QFileInfo>
#include <QRegularExpression>
#include <QThread>
#include <QElapsedTimer>
#include <QScrollBar>


Editor::Editor(QWidget *parent)
    : Editor(nullptr, {nullptr}, parent)
{}

Editor::Editor(TextStream* stream, std::unique_ptr<QFile> file_p, QWidget *parent)
    : QPlainTextEdit(parent)
    , m_name{SETTINGS.defaultDocName}
    , m_textStreamThread{nullptr}
    , m_textStream{stream}
    , m_file{std::move(file_p)}
    , m_encoding{QStringConverter::Utf8}
    , m_endOfLine{EndOfLine::UNAVAILABLE}
    , m_hasBom{false}
    , m_format{}
    , m_search{}
    , highLighter{new Highlighter(this->document())}
    , m_aborted{false}
    , m_loadingInProgress{false}
    , m_reloading{false}
    , m_loadingPos{0}
{
    if(m_file)
    {
        m_name = QFileInfo(*m_file).fileName();
    }

    /// QSyntaxHighlighter::rehighlight() does emit contentsChanged for some reason but not contentsChange
    /// We want contentsChange because only real edits should trigger it
    connect(document(), &QTextDocument::contentsChange, this, &Editor::onContentsChange);

    if(m_textStream)
    {
        auto conn_type = Qt::AutoConnection;
        if(m_textStream->thread() != thread())
        {
            m_textStreamThread = m_textStream->thread();
            conn_type = Qt::QueuedConnection;
        }

        connect(m_textStream, &TextStream::dataQueued, this, &Editor::onDataQueued, conn_type);
    }
}

Editor::~Editor()
{
    /// In case destroyed while file loading was in progress
    if(!m_aborted)
    {
        abortTasks();
    }
    if(m_textStreamThread)
    {
        /// No need to ask isRunning() before calling wait()
        const bool finished = m_textStreamThread->wait(1000);
        qDebug() << "textStream thread finished:" << finished;
        if(!finished)
        {
            qWarning() << m_name << "worker thread did not exit cleanly";
            m_textStreamThread->terminate();
        }
    }
}

void Editor::abortTasks()
{
    qDebug() << "abortTasks";
    m_aborted = true;

    highLighter->abort();

    if(m_textStream)
    {
        /// m_textStream QPointer will be automatically set to null after it was destroyed
        m_textStream->quit();
        m_textStream->deleteLater();
    }
    if(m_textStreamThread)
    {
        m_textStreamThread->quit();
    }
}

/// Handle line endings here and stream to file using binary mode.
/// If writing to file in text mode, it would insert platform specific end of lines automatically.
QString Editor::toPlainText() const
{
    /// Note: toPlainText() converts the internally used unicode paragraph separators to \n
    /// toRawText() returns the unicode versions directly
    /// Let's let toPlainText to handle the unicode chars and we just replace the \n's here
    using namespace Qt::StringLiterals;
    QString txt = document()->toPlainText();
    if(m_endOfLine == EndOfLine::WINDOWS)
    {
        /// NOTE not performance tested. But Qt uses similar internally.
        txt.replace(u'\n', "\r\n"_L1); /// u is utf16 literal, _L1 is latin-1 literal
    }
    else if(m_endOfLine == EndOfLine::MAC)
    {
        txt.replace(u'\n', '\r'_L1); /// u is utf16 literal, _L1 is latin-1 literal
    }
    return txt;
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

    auto [tStream, thread] = createStreamAndThread(fileName);
    Editor* editor = new Editor(tStream, std::move(file_p), parent);

    editor->m_loadingInProgress = true;

    /// QThread enters its own event loop here which executes until exit is called (or quit)
    editor->m_start_t = std::chrono::high_resolution_clock::now(); /// DEBUG
    thread->start();

    return editor;
}

/// static
std::pair<TextStream*, QThread*> Editor::createStreamAndThread(const QString& fileName)
{
    /// Asynchronous, transfer the data via signals
    /// This TextStream will be deleted when the file loading is finished
    TextStream* tStream = new TextStream(fileName);
    tStream->setEncoding(QStringConverter::Encoding::Utf8);
    tStream->setAutoDetectUnicode(true);
    tStream->setAutoDetectBom(true);
    tStream->setValidateUtf(true); /// Does not really work for utf16
    tStream->setValidateLatin(true);

    /// This thread will be deleted when the file loading is finished
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
    connect(thread, &QThread::started, tStream, &TextStream::readChunks);
    /// Stop the thread's event loop when tStream is deleted
    /// The thread must outlive the objects it owns
    connect(tStream, &TextStream::destroyed, thread, &QThread::quit);
    /// Delete the thread after it has finished
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    return { tStream, thread };
}

void Editor::onDataQueued()
{
    if(m_aborted || !m_textStream)
    {
        return;
    }

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
        m_endOfLine = meta.endOfLine; /// TODO: Maybe if this remains unknown after the initial check, this should be checked later on (if the first EOL is very far in the doc)
        m_hasBom = meta.hasBom;

        if(document()->isModified())
        {
            document()->setModified(false);
            emit modificationChanged(false);
        }
        emit undoAvailable(false);
        emit redoAvailable(false);

        setReadOnly(true);
        blockSignals(true); /// Not 100% sure, should we set the signals enabled on the last iteration?
        document()->blockSignals(true);
        setUndoRedoEnabled(false);

        /// Small optimization because QTextCursor::insertText(const QString &text) copies the format on every call
        m_format = cursor.charFormat();
    }

    /// Store user's cursor location before we do edits
    auto userCursor = textCursor();
    const auto userAnchor = userCursor.anchor();
    const auto userPosition = userCursor.position();
    /// Scrollbar position needs to be saved because setTextCursor will reset the scrolling to make cursor visible.
    /// Probably would need to hack QPlainTextEdit to prevent it.
    const auto scrollbarPrevValue = verticalScrollBar()->value();

    /// NOTE insertText converts any line ending characters to unicode block separators.
    /// So as long as we use insertText, we can't choose what eol type we show in the document.
    /// Furthermore I don't know if it's even possible to use ascii line endings in QTextDocument
    if(!m_reloading)
    {
        cursor.movePosition(QTextCursor::End); /// Ensure we are appending to the end
        cursor.insertText(dataChunk, m_format);
    }
    else
    {
        /// setPosition out of range possible here (different end of lines)
        /// dataChunk and document end of lines must match
        const int chunkEnd = qMin(m_loadingPos + dataChunk.length(), document()->characterCount() - 1);
        cursor.setPosition(m_loadingPos, QTextCursor::MoveAnchor);
        cursor.setPosition(chunkEnd, QTextCursor::KeepAnchor);
//        if(cursor.selectedText() != dataChunk) /// This check is a bit slow
        {
            cursor.insertText(dataChunk, m_format);
        }
    }
    document()->setModified(false); /// insertText sets modified flag to true

    m_loadingPos += dataChunk.length();
    blockSignals(false);
    emit dataLoadingUpdate(m_loadingPos); /// Blocksignals must be false

    /// Adapt the text insert amount to desired UI update rate
    /// Insert largest possible blocks because it's faster, but without exceeding frame time
    const qint64 elapsed = timer.elapsed();
//    static constexpr qint64 FRAME_TIME_TARGET = 10; /// ~100 FPS Higher update rate impacts the total time but not a lot
    static constexpr qint64 FRAME_TIME_TARGET = 16; /// ~60 FPS Feels quite smooth, though not 120Hz display smooth
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

    /// Retrieve the user's wanted cursor position after the insert operations
    userCursor.setPosition(userAnchor, QTextCursor::MoveAnchor);
    userCursor.setPosition(userPosition, QTextCursor::KeepAnchor);
    setTextCursor(userCursor); /// calls ensureCursorVisible()
    verticalScrollBar()->setValue(scrollbarPrevValue);

    /// This gets processed only when all the signals are processed, even if the data queue went empty way earlier
    if(Q_UNLIKELY(meta.done))
    {
        if(m_reloading)
        {
            /// Truncate excess text from the doc if the reloaded text was shorter
            QTextCursor cursor_trunc(document());
            cursor_trunc.setPosition(m_loadingPos);
            if(!cursor_trunc.atEnd())
            {
                cursor_trunc.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                cursor_trunc.removeSelectedText();
            }
        }

        m_end_t = std::chrono::high_resolution_clock::now(); /// DEBUG
        qInfo() << "insertText DONE in" << std::chrono::duration_cast<std::chrono::milliseconds>(m_end_t-m_start_t).count();
        qDebug() << "Final batchSize" << m_textStream->batchSize();

        /// 10000 = 5,3s
        /// 20000 = 3,1s
        /// 100000 = 1,4s

        document()->setModified(false);
        document()->blockSignals(false);
        blockSignals(false);
        setReadOnly(false);

        /// Settings
        setUndoRedoEnabled(true);

        emit dataLoadingFinished();

        /// The purpose is completed
        m_textStream->deleteLater();

        m_loadingInProgress = false;
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
    const bool had_file = m_file != nullptr;
    m_file.reset();
    auto file = std::make_unique<QFile>(fileName);
    File::Status saved = File::saveFile(toPlainText(), *file, m_encoding, m_hasBom);

    if(saved == File::Status::SUCCESS_WRITE)
    {
        m_file = std::move(file);
        setName(QFileInfo(*m_file).fileName());
        document()->setModified(false);
    }
    const bool has_file = m_file != nullptr;
    if(had_file != has_file)
    {
        emit hasFileChanged(has_file);
    }
    return saved;
}

void Editor::reload()
{
    qDebug() << "reload";
    if(m_file == nullptr)
    {
        qDebug() << "No file";
        return;
    }
    if(m_loadingInProgress)
    {
        qDebug() << "Already reloading";
        return;
    }
    m_loadingInProgress = true;

    std::tie(m_textStream, m_textStreamThread) = createStreamAndThread(m_file->fileName());

    connect(m_textStream, &TextStream::dataQueued, this, &Editor::onDataQueued,
            m_textStream->thread() == this->thread() ? Qt::AutoConnection : Qt::QueuedConnection);

    m_reloading = true;
    m_loadingPos = 0;

    /// QThread enters its own event loop here which executes until exit is called (or quit)
    m_start_t = std::chrono::high_resolution_clock::now(); /// DEBUG
    m_textStreamThread->start();
}

qsizetype Editor::getMatchCount(const QString& sterm, QTextDocument::FindFlags flags)
{
    qDebug() << "getMatchCount" << sterm;

    QMutexLocker lock(&m_searchMutex);
    if(m_search.m_countInProgress)
    {
        return m_search.matchCount; /// Return old value
        /// TODO Should test if caching and reusing the old value can cause problems
    }

    flags.setFlag(QTextDocument::FindBackward, false); /// Don't store search direction

    if(sterm != m_search.term || flags != m_search.flags)
    {
        m_search.term = sterm;
        m_search.flags = flags;
        m_search.m_countInProgress = true;
        /// Tää documentin haku ei ehkä oo ihan thread safe
        const auto doc = document()->toRawText();
//        const auto doc = document()->toPlainText(); /// Check later if should use this if toRawText has issues with unicode or something
        const QStringView doc_view(doc);

        lock.unlock();
        const auto match_count = Search::countMatches(doc_view, m_search.term, m_search.flags);
        lock.relock();
        m_search.matchCount = match_count;
        m_search.m_countInProgress = false;
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

QString Editor::filePath() const
{
    return m_file ? QDir::toNativeSeparators(m_file->fileName()) : "";
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

QString Editor::endOfLineName() const
{
    return TextStream::nameForEndOfLine(m_endOfLine);
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
    /// fontMetrics().averageCharWidth() was not accurate on Ubuntu even when the font was supposed to be monospace.
    /// Qt seems to calculate char widths in fixed point in QFontMetricsF, causing inaccuracy for some values.
    /// Give more than one character to improve accuracy.
    const auto metricsF = QFontMetricsF(this->font());
    const qreal space_width_4 = metricsF.horizontalAdvance("    ") / 4.0; /// Seems accurate on windows and ubuntu
    setTabStopDistance(SETTINGS.tabWidthChars * space_width_4);
}

void Editor::setFont(const QFont& font)
{
    QWidget::setFont(font);
    /// changeEvent handles updateTabWidth
}

void Editor::setHighlightRegex(const QString& regexStr, QTextDocument::FindFlags flags)
{
    highLighter->setRegex(regexStr, flags, firstVisibleBlock().position());
}

void Editor::setHighlighterEnabled(bool enabled)
{
    qDebug() << "setHighlighterEnabled" << enabled;
    highLighter->setDocument(enabled ? document() : nullptr);
    if(!enabled)
    {
        highLighter->setRegex("");
    }
}

void Editor::onContentsChange([[maybe_unused]]int position, [[maybe_unused]]int charsRemoved, [[maybe_unused]]int charsAdded)
{
//    qDebug() << "onContentsChange" << position << "" << charsRemoved << "" << charsAdded;
    invalidateSearchResults();
}

void Editor::invalidateSearchResults()
{
    qDebug() << "invalidateSearchResults";
    m_search.invalidate();
}

void Editor::changeEvent(QEvent* e)
{
    if(e->type() == QEvent::FontChange)
    {
        updateTabWidth();
    }
    QPlainTextEdit::changeEvent(e);
}

void Editor::wheelEvent(QWheelEvent* e)
{
    /// QPlainTextEdit::wheelEvent for some reason does not let zooming to happen unless we are in readOnly mode
    if(e->modifiers() & Qt::ControlModifier)
    {
        /// Delta is mostly 1 or -1, but can be larger if multiple wheel events have been stacked together
        const float delta = e->angleDelta().y() / 120.f;
        SETTINGS.incrementFontSize(delta);
        return;
    }
    /// Call the base class implementation if we did NOT handle the event
    QPlainTextEdit::wheelEvent(e);
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

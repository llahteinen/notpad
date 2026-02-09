#include "editor.hpp"
#include "utils/textstream.h"
#include "settings.hpp"
#include <QFileDialog>
#include <QFileInfo>
#include <QRegularExpression>


Editor::Editor(QWidget *parent)
    : Editor({}, {nullptr}, parent)
{}

Editor::Editor(const QString& text, std::unique_ptr<QFile> file_p, QWidget *parent)
    : QPlainTextEdit(text, parent)
    , m_name{SETTINGS.defaultDocName}
    , m_file{std::move(file_p)}
    , m_encoding{QStringConverter::Utf8}
    , m_hasBom{false}
    , m_search{}
{
    if(m_file)
    {
        m_name = QFileInfo(*m_file).fileName();
    }

    connect(this, &Editor::textChanged, this, &Editor::invalidateSearchResults);
}

/// static
Editor* Editor::createEditor(File::Status& o_status, const QString& fileName, QWidget* parent)
{
    auto file_p = std::make_unique<QFile>();
    o_status = File::openFile(*file_p, fileName);
    if(o_status != File::Status::SUCCESS_READ)
    {
        return nullptr;
    }

    TextStream fileStream(file_p.get());
    fileStream.setEncoding(QStringConverter::Encoding::Utf8);
    fileStream.setAutoDetectUnicode(true);
    fileStream.setAutoDetectBom(true);
    fileStream.setValidateUtf(true);
    fileStream.setValidateLatin(true);
    Editor* editor = new Editor(fileStream.readAll(), std::move(file_p), parent);
    editor->m_encoding = fileStream.encoding();
    editor->m_hasBom = fileStream.hasBom();
    qDebug() << "encoding" << QStringConverter::nameForEncoding(editor->m_encoding);
    /// file_p is nullptr now
    fileStream.device()->close();
    return editor;
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

void Editor::invalidateSearchResults()
{
    qDebug() << "invalidateSearchResults";
    m_search = {};
}

qsizetype Editor::countMatches(const QString& sterm, QTextDocument::FindFlags flags)
{
    qDebug() << "countMatches";
    QRegularExpression::PatternOptions reg_opt{};
    /// NOTE: Case insensitive is default for QTextDocument::FindFlags
    if(!flags.testFlag(QTextDocument::FindFlag::FindCaseSensitively))
    {
        reg_opt.setFlag(QRegularExpression::PatternOption::CaseInsensitiveOption);
    }
    /// Else find case sensitively
    qDebug() << "regex flags" << reg_opt;

    const auto doc = document()->toRawText();
//    const auto doc = document()->toPlainText(); /// Check later if should use this if toRawText has issues with unicode or something

    /// Regex method (this is faster than QString::count(QString))
    const QRegularExpression reg{QRegularExpression::escape(sterm), reg_opt}; /// Normal string-like search using regex
//    const QRegularExpression reg{sterm}; /// Full regex pattern search. Should check PatternOptions at some point
//    reg.optimize(); /// Not sure what this does, does not seem to affect performance
    if(!reg.isValid())
    {
        qInfo() << "Invalid regexp:" << reg.errorString();
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

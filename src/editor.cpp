#include "editor.hpp"
#include "utils/textstream.h"
#include "settings.hpp"
#include <QFileDialog>
#include <QFileInfo>


Editor::Editor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_name{SETTINGS.defaultDocName}
    , m_file{}
    , m_encoding{QStringConverter::Utf8}
    , m_hasBom{false}
{}

Editor::Editor(const QString& text, std::unique_ptr<QFile> file_p, QWidget *parent)
    : QPlainTextEdit(text, parent)
    , m_name{SETTINGS.defaultDocName}
    , m_file{std::move(file_p)}
{
    m_name = QFileInfo(*m_file).fileName();
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
    File::Status saved = File::saveFile(toPlainText(), *m_file);

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
    File::Status saved = File::saveFile(toPlainText(), *file);

    if(saved == File::Status::SUCCESS_WRITE)
    {
        m_file = std::move(file);
        setName(QFileInfo(*m_file).fileName());
        document()->setModified(false);
    }
    return saved;
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

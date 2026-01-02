#include "editor.hpp"
#include <QFileDialog>


Editor::Editor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_file{}
{}

Editor::Editor(const QString& text, std::unique_ptr<QFile> file_p, QWidget *parent)
    : QPlainTextEdit(text, parent)
    , m_file{std::move(file_p)}
{}

/// static
Editor* Editor::createEditor(File::Status& o_status, const QString& fileName, QWidget* parent)
{
    auto file_p = std::make_unique<QFile>();
    o_status = File::openFile(*file_p, fileName);
    if(o_status != File::Status::SUCCESS_READ)
    {
        return nullptr;
    }

    QTextStream fileStream(file_p.get());
    Editor* editor = new Editor(fileStream.readAll(), std::move(file_p), parent);
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
        document()->setModified(false);
    }
    return saved;
}

bool Editor::isModified() const
{
    return document()->isModified();
}

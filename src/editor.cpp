#include "editor.hpp"
#include "settings.hpp"
#include "file.hpp"
#include <QFileDialog>


bool Editor::save()
{
    qDebug() << "Editor::save";
    bool saved = false;
    if(!m_file)
    {
        saved = saveAs();
    }
    else
    {
        /// Note: toPlainText() creates a copy of all the text inside the QPlainTextEdit
        saved = File::saveFile(toPlainText(), m_file.get());
    }

    if(saved)
    {
        document()->setModified(false);
    }
    return saved;
}

bool Editor::saveAs()
{
    qDebug() << "Editor::saveAs";
    QFileDialog fileDialog(this, tr("Save Document"), SETTINGS.currentDir.absolutePath());
    fileDialog.setOptions(QFileDialog::DontUseNativeDialog);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setViewMode(QFileDialog::ViewMode::Detail);
//    fileDialog.setDefaultSuffix("txt");   /// TODO: Make selectable?
    fileDialog.setFileMode(QFileDialog::FileMode::AnyFile);
    fileDialog.setNameFilters(SETTINGS.nameFilters);
    fileDialog.selectNameFilter(SETTINGS.currentNameFilter);

    bool saved = false;
    if(fileDialog.exec() == QDialog::Accepted)
    {
        SETTINGS.currentNameFilter = fileDialog.selectedNameFilter();

        qDebug() << fileDialog.selectedFiles();
        Q_ASSERT(fileDialog.selectedFiles().size() == 1 && "Selected save file count must be 1");
        saved = File::saveFile(m_file, toPlainText(), fileDialog.selectedFiles().at(0));
    }

    if(saved)
    {
        document()->setModified(false);
    }
    return saved;
}

bool Editor::isModified() const
{
    return document()->isModified();
}

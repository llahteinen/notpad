#include "editor.hpp"
#include "settings.hpp"
#include <QFileDialog>


File::Status Editor::save()
{
    qDebug() << "Editor::save";
    File::Status saved{File::Status::UNKNOWN};
    if(!m_file)
    {
        saved = saveAs();
    }
    else
    {
        /// Note: toPlainText() creates a copy of all the text inside the QPlainTextEdit
        saved = File::saveFile(toPlainText(), m_file.get());
    }

    if(saved == File::Status::SUCCESS_WRITE)
    {
        document()->setModified(false);
    }
    return saved;
}

File::Status Editor::saveAs()
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

    File::Status saved{File::Status::UNKNOWN};
    if(fileDialog.exec() == QDialog::Accepted)
    {
        SETTINGS.currentNameFilter = fileDialog.selectedNameFilter();

        qDebug() << fileDialog.selectedFiles();
        Q_ASSERT(fileDialog.selectedFiles().size() == 1 && "Selected save file count must be 1");
        saved = File::saveFile(m_file, toPlainText(), fileDialog.selectedFiles().at(0));
    }
    else
    {
        saved = File::Status::CANCELED;
    }

    if(saved == File::Status::SUCCESS_WRITE)
    {
        document()->setModified(false);
    }
    return saved;
}

bool Editor::isModified() const
{
    return document()->isModified();
}

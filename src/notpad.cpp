#include "notpad.hpp"
#include "forms/ui_notpad.h"
#include "tab.hpp"
#include "editor.hpp"
#include "file.hpp"
#include "settings.hpp"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QSettings>



NotPad::NotPad(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NotPad)
    , m_tabManager{}
    , m_editor{}
    , m_prevEditor{}
{
    qInfo() << PROJECT_NAME << "starting";

    ui->setupUi(this);
    m_tabManager = ui->tabWidget;
    m_tabManager->setupUi();
    /// Close the template tabs
    while(m_tabManager->count() > 0)
        m_tabManager->removeTab(0);

    connect(m_tabManager, &TabManager::currentChanged, this, &NotPad::onCurrentTabChanged);
    connect(m_tabManager, &TabManager::tabCloseRequested, this, &NotPad::onTabCloseRequested);

    QString project_name{PROJECT_NAME};
#if defined(QT_DEBUG)
    project_name.append("_dbg");
#endif
    setWindowTitle(QString("%1 v%2").arg(project_name, PROJECT_VERSION));

    QCoreApplication::setOrganizationName(ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(project_name);

    QFile styleFile(":/forms/styles.css");
    if(styleFile.open(QFile::ReadOnly))
    {
        const auto style = styleFile.readAll();
//        qDebug() << "style" << style;
        qApp->setStyleSheet(style);
        if(style.trimmed().isEmpty())
        {
            qWarning() << "Style was empty.";
        }
    }
    else
    {
        qWarning() << "Setting style failed.";
    }

    on_actionFind_triggered(ui->actionFind->isChecked());

    qDebug() << "Platform:" << QGuiApplication::platformName();
    qDebug() << "Available XDG themes:" << QIcon::themeSearchPaths();
    qDebug() << "Current theme:" << QIcon::themeName();


    /// Setup open tabs
    loadSettings();
    SETTINGS.pers.startupCounter++;
    qDebug() << "startups" << SETTINGS.pers.startupCounter;
    /// Check command line arguments
    handleArguments();
    /// Add empty tab if there is none
    if(m_tabManager->count() == 0) m_tabManager->addEmptyTab();

//    SETTINGS.currentDir = QDir("../../../testifiles"); /// Set save/load dialog starting location
}

NotPad::~NotPad()
{
    delete ui;
}

void NotPad::closeEvent(QCloseEvent* event)
{
    qDebug() << "MainWindow closeEvent";
    if((!SETTINGS.confirmAppClose || confirmAppClose(tr("Quitting")))
        && closeAllTabs())
    {
        saveSettings();
        QMainWindow::closeEvent(event);
    }
    else
    {
        /// Don't close
        event->ignore();
    }
}

void NotPad::saveSettings()
{
    SETTINGS.pers.windowGeometry = saveGeometry();

    QSettings settings;
    SETTINGS.pers.toQSettings(settings);
}

void NotPad::loadSettings()
{
    /// Load settings from persistent storage
    const QSettings settings;
    SETTINGS.pers.fromQSettings(settings);

    /// Apply settings
    if(!SETTINGS.pers.windowGeometry.isEmpty()) restoreGeometry(SETTINGS.pers.windowGeometry); /// Seems to work even if the data is something weird

    /// Load previous session
    /// TODO: restore the active tab?
    qDebug() << "sessionTabs" << SETTINGS.pers.sessionTabs;
    /// Check if the files exist?
    openFiles(SETTINGS.pers.sessionTabs);
}

void NotPad::handleArguments()
{
    /// arg0    = path to this executable
    /// arg1... = possible file
    /// Note that Qt automatically removes it's own supported args such as -widgetcount
    const auto arguments = qApp->arguments();
//    qDebug() << "args" << arguments;
    QStringList files;
    for(int i = 1; i < arguments.size(); ++i)
    {
        /// Check if we have files
        const QFileInfo arg{arguments.at(i)};
        if(arg.isFile())
        {
            qInfo() << "Got file argument" << arg.filePath();
            files.append(arg.absoluteFilePath());
        }
    }
    openFiles(files);
}

void NotPad::persistCurrentTabs()
{
    QStringList files;
    const auto count = m_tabManager->count();
    qDebug() << "persistCurrentTabs" << count;
    for(int i = 0; i < count; ++i)
    {
        const QFile* file_p = m_tabManager->widget(i)->file();
        if(file_p == nullptr) continue;

        QFileInfo fi{*file_p};
        files.append(fi.absoluteFilePath());
    }
    qDebug() << files;
    SETTINGS.pers.sessionTabs = files;
}

bool NotPad::closeAllTabs()
{
    /// 1. Save or discard modified tabs
    if(!cleanupModifiedTabs())
    {
        qDebug() << "closeAllTabs abort";
        return false;
    }

    /// TODO: Pitäisi ehkä olla sittenkin niin että jos tabilla on tiedosto, niin se persistoidaan silti vaikka olisi laitettu discard
    /// 2. Persist remaining (saved) tabs as a session
    persistCurrentTabs();

    /// 3. Close all remaining tabs
    auto count = m_tabManager->count();
    for(int i = 0; i < count; ++i)
    {
        /// Always close the active tab
        if(onTabCloseRequested(m_tabManager->currentIndex()))
        {
            qApp->processEvents(); /// So that the UI briefly displays the new state before the whole window closes if this was the last tab
        }
        else
        {
            qDebug() << "closeAllTabs abort";
            return false;
        }
    }

    return true;
}

bool NotPad::saveOrCloseTab(Editor* editor)
{
    Q_ASSERT(editor != nullptr);
    /// If file is modified, ask save or discard
    /// Note special case: empty tab (not modified and no file)
    if(editor->isModified() || editor->file() == nullptr)
    {
        const int index = m_tabManager->indexOf(editor);
        m_tabManager->setCurrentIndex(index); /// Bring the tab to foreground for the user to see

        /// This will save the file and return true
        /// OR discard it and return true
        /// OR cancel and return false
        if(confirmFileClose(editor, editor->name()))
        {
            /// If the tab still isModified OR does not have a file, means permission to discard
            if(editor->isModified() || editor->file() == nullptr) m_tabManager->closeTab(index);
            return true;
        }
        else
        {
            return false; /// abort (Cancel pressed)
        }
    }
    /// Did not need saving or closing (was unmodified)
    return true;
}

bool NotPad::cleanupModifiedTabs()
{
    if(m_tabManager->count() == 0)
    {
        return true;
    }

    /// Ask whether to save or discard modified unsaved tabs
    ///  Discarded will be closed, saved will be left open
    /// Unmodified tabs will be left open and untouched

    /// Process the active tab first
    {
        if(saveOrCloseTab(m_tabManager->currentWidget()))
        {
            qApp->processEvents(); /// So that the UI briefly displays the new state before the whole window closes if this was the last tab
        }
        else
        {
            return false; /// abort (Cancel pressed)
        }
    }
    /// Process all remaining tabs
    auto count = m_tabManager->count();
    for(int i = count-1; i >= 0; --i)
    {
        if(saveOrCloseTab(m_tabManager->widget(i)))
        {
            qApp->processEvents(); /// So that the UI briefly displays the new state before the whole window closes if this was the last tab
        }
        else
        {
            return false; /// abort (Cancel pressed)
        }
    }
    return true;
}

bool NotPad::onTabCloseRequested(int index)
{
    bool permission = false;
    Editor* editor = m_tabManager->widget(index);
    Q_ASSERT(editor != nullptr);
    if(editor != nullptr)
    {
        permission = confirmFileClose(editor, editor->name());
        if(permission)
        {
            m_tabManager->closeTab(index);
        }
        else
        {
            /// Don't close
        }
    }
    return permission;
}

void NotPad::onCurrentTabChanged(int index)
{
    qDebug() << "onCurrentTabChanged" << index;
    m_prevEditor = m_editor;

    /// Last tab closed
    if(index == -1)
    {
        /// Close application after last tab?
        /// TODO: Should gray out everything in menus etc if we support window with zero open tabs
        if(true)
        {
            qApp->quit(); /// This will not execute directly, so need to call return
            return;
        }
    }

    m_editor = m_tabManager->currentWidget();
    if(m_editor == nullptr)
    {
        qDebug() << "No editor";
        /// No more tabs left
    }
    qDebug() << "file" << currentFile();

    setupSignals();
    setupMenu();
}

void NotPad::setupSignals()
{
    qDebug() << "setupSignals" << m_prevEditor << m_editor;
    if(m_prevEditor)
    {
        disconnect(m_prevEditor, &QPlainTextEdit::undoAvailable, this, &NotPad::onUndoAvailable);
        disconnect(m_prevEditor, &QPlainTextEdit::redoAvailable, this, &NotPad::onRedoAvailable);
        disconnect(m_prevEditor, &QPlainTextEdit::textChanged,   this, &NotPad::onTextChanged);
    }
    if(m_editor)
    {
        connect(m_editor, &QPlainTextEdit::undoAvailable, this, &NotPad::onUndoAvailable, Qt::UniqueConnection);
        connect(m_editor, &QPlainTextEdit::redoAvailable, this, &NotPad::onRedoAvailable, Qt::UniqueConnection);
        connect(m_editor, &QPlainTextEdit::textChanged,   this, &NotPad::onTextChanged,   Qt::UniqueConnection);
    }
}

void NotPad::setupMenu()
{
    if(m_editor)
    {
        onUndoAvailable(m_editor->document()->isUndoAvailable());
        onRedoAvailable(m_editor->document()->isRedoAvailable());
        ui->actionWord_wrap->setChecked(m_editor->isWordWrap());
//        ui->actionSave->setEnabled(m_editor->isModified());
    }
    else
    {
        onUndoAvailable(false);
        onRedoAvailable(false);
        ui->actionWord_wrap->setChecked(SETTINGS.pers.wordWrap);
    }
}

int NotPad::incrementFontSize(int increment)
{
    auto font = m_editor->font();
    auto size = font.pointSize();
    auto index = SETTINGS.standardFontSizes.indexOf(size);
    Q_ASSERT(index >= 0);
    if(index < 0)
    {
        qWarning() << "Got weird font size" << size;
        index = SETTINGS.standardFontSizes.indexOf(SETTINGS.fontSizeDefault);
    }
    index += increment;
    index = qMax(index, 0);
    index = qMin(index, SETTINGS.standardFontSizes.length()-1);
    qDebug() << "index" << index;
    size = SETTINGS.standardFontSizes.at(index);
    qDebug() << "size" << size;
    font.setPointSize(size);
    m_editor->setFont(font);
    updateTabWidth(); /// TODO: Implement using QEvent::FontChange
    return size;
}

int NotPad::restoreFontSize()
{
    auto font = m_editor->font();
    auto size = SETTINGS.fontSizeDefault;
    font.setPointSize(size);
    qDebug() << "pointSize" << font.pointSize();
    m_editor->setFont(font);
    updateTabWidth();
    return size;
}

void NotPad::updateTabWidth()
{
    m_editor->setTabStopDistance(SETTINGS.tabWidthChars * m_editor->fontMetrics().averageCharWidth());
}

void NotPad::messageOpenStatus(const File::Status& status)
{
    QString msg;
    switch(status)
    {
    case File::Status::CANCELED:
        break;
    case File::Status::FAIL_OPEN_NOTFOUND:
        msg = tr("File not found: %1").arg(QDir::toNativeSeparators(status.fileName));
        break;
    case File::Status::FAIL_OPEN_READ:
        msg = tr("Cannot open file for reading: %1").arg(status.errorString);
        break;
    case File::Status::SUCCESS_READ:
        msg = tr("File opened: %1").arg(QDir::toNativeSeparators(status.fileName));
        break;
    default:
        msg = tr("File open/read failed.");
    }
    statusBar()->showMessage(msg);
}

void NotPad::openFiles(const QStringList &fileNameList)
{
//    qDebug() << "fileNameList" << fileNameList;
    for(const auto& fname : fileNameList)
    {
        if(openFile(fname))
        {
            /// The last one in the list will dictate the current dir
            SETTINGS.currentDir = QFileInfo(fname).dir();
        }
    }
    /// TODO: Check if a file is already open? Or allow multiple same files?
}

bool NotPad::openFile(const QString &fileName)
{
    const auto status = m_tabManager->addTabFromFile(fileName);
    qDebug() << "openFile status" << static_cast<int>(status);
    messageOpenStatus(status);
    return status == File::Status::SUCCESS_READ;
}

void NotPad::messageSaveStatus(const File::Status& status)
{
    QString msg;
    switch(status)
    {
    case File::Status::CANCELED:
        break;
    case File::Status::FAIL_OPEN_WRITE:
        msg = tr("Cannot open file for writing: %1").arg(status.errorString);
        break;
    case File::Status::FAIL_WRITE:
        msg = tr("File write failed: %1").arg(status.errorString);
        break;
    case File::Status::FAIL_WRITE_UNKNOWN:
        msg = tr("File write failed: %1").arg(status.errorString);
        break;
    case File::Status::SUCCESS_WRITE:
        msg = tr("File saved: %1").arg(QDir::toNativeSeparators(status.fileName));
        break;
    default:
        msg = tr("File write failed.");
    }
    statusBar()->showMessage(msg);
}

bool NotPad::save()
{
    return save(m_editor);
}

bool NotPad::save(Editor* const editor)
{
    qDebug() << "NotPad::save";
    if(!editor->saveOrSaveAs())
    {
        return saveAs();
    }
    const auto status = editor->save();
    messageSaveStatus(status);
    return status == File::Status::SUCCESS_WRITE;
}

bool NotPad::saveAs()
{
    qDebug() << "NotPad::saveAs";
    QString start_path = SETTINGS.currentDir.absolutePath();
    QString name_filter = SETTINGS.currentNameFilter;
    if(m_editor->file() != nullptr) /// TODO: duplikaattikoodia openissa
    {
        const auto fi = QFileInfo(*m_editor->file());
        start_path = fi.absoluteFilePath(); /// Path including file name
        name_filter = SETTINGS.nameFilters.getFilter(fi.suffix(), SETTINGS.nameFilters.first());
    }
//    qDebug() << "start_path name_filter" << start_path << name_filter;

    QFileDialog fileDialog(this, tr("Save Document"), start_path);
    fileDialog.setOptions(QFileDialog::DontUseNativeDialog);
    /// Native dialog automatically appends file extension, but the Qt dialog doesn't unless setDefaultSuffix is set
    /// "Do you want to overwrite?" we need to have the suffix set here already -> use setDefaultSuffix
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setViewMode(QFileDialog::ViewMode::Detail); /// TODO: tallenna (voi varmaan käyttää saveState())
    fileDialog.setFileMode(QFileDialog::FileMode::AnyFile);
    fileDialog.setNameFilters(SETTINGS.nameFilters);
    fileDialog.selectNameFilter(name_filter);
    fileDialog.setDefaultSuffix(NameFilterList::getSuffix(name_filter));

    connect(&fileDialog, &QFileDialog::filterSelected, this,
            [&fileDialog](const QString& filter)
            {
                fileDialog.setDefaultSuffix(NameFilterList::getSuffix(filter));
            });

    File::Status status{File::Status::UNKNOWN};
    if(fileDialog.exec() == QDialog::Accepted)
    {
        qDebug() << fileDialog.selectedFiles();
        Q_ASSERT(fileDialog.selectedFiles().size() == 1 && "Selected save file count must be 1");
        const QString file_name = fileDialog.selectedFiles().constFirst();
        status = m_editor->saveAs(file_name);

        const QFileInfo fileInfo(file_name);
        SETTINGS.currentDir = fileInfo.dir();
//        SETTINGS.currentDir = fileInfo.absoluteDir();
        SETTINGS.currentNameFilter = fileDialog.selectedNameFilter();
    }
    else
    {
        status.code = File::Status::CANCELED;
    }

    messageSaveStatus(status);
    return status == File::Status::SUCCESS_WRITE;
}

bool NotPad::confirmAppClose(const QString& messageTitle)
{
    bool permission = false;
    {
        const auto choice = QMessageBox::warning(this, messageTitle,
                                                 tr("Do you want to quit?"),
                                                 QMessageBox::Yes | QMessageBox::No,
                                                 QMessageBox::No);

        switch(choice)
        {
        case QMessageBox::Yes:
            {
                permission = true;
                break;
            }
        case QMessageBox::No:
        default:
            {
                permission = false;
            }
        }
    }
    return permission;
}

bool NotPad::confirmFileClose(Editor* editor, const QString& messageTitle)
{
    qDebug() << "NotPad::confirmFileClose";
    bool permission = false;
    if(editor->isModified())
    {
        qDebug() << "File is edited";
        const auto choice = QMessageBox::warning(this, messageTitle,
                                                 tr("The document has been modified.\n"
                                                    "Do you want to save your changes?"),
                                                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                                 QMessageBox::Save);

        qDebug() << "Choice" << choice;
        switch(choice)
        {
        case QMessageBox::Discard:
            {
                permission = true;
                break;
            }
        case QMessageBox::Save:
            {
                /// true: file was saved
                /// false: file saving canceled or failed
                permission = save(editor);
                break;
            }
        case QMessageBox::Cancel:
        default:
            {
                permission = false;
            }
        }
    }
    else
    {
        /// Not edited, always permission to close
        permission = true;
    }
    return permission;
}

const QFile* NotPad::currentFile()
{
    if(m_editor != nullptr)
    {
        return m_editor->file();
    }
    return nullptr;
}

/// SLOTS ================================================

void NotPad::on_actionNew_triggered()
{
    qDebug() << "on_actionNew_triggered";

    if(m_tabManager->count() == 0)
    {
        m_tabManager->addEmptyTab();
        /// onCurrentTabChanged will be triggered
    }
    else if(m_tabManager->count() > 0)
    {
        /// Close current tab and open empty one
        if(confirmFileClose(m_editor, tr("New file")))
        {
            m_tabManager->resetTab(m_tabManager->currentIndex());
        }
    }
}

void NotPad::on_actionNewTab_triggered()
{
    m_tabManager->addEmptyTab();
}

void NotPad::on_actionOpen_triggered()
{
    qDebug() << "on_actionOpen_triggered";

    QString start_path = SETTINGS.currentDir.absolutePath();
    QString name_filter = SETTINGS.currentNameFilter;
    /// If there is a tab open that has a saved file, use that file path and suffix
    if(m_editor->file() != nullptr)
    {
        const auto fi = QFileInfo(*m_editor->file());
        start_path = fi.absolutePath(); /// Path not including file name
        name_filter = SETTINGS.nameFilters.getFilter(fi.suffix(), SETTINGS.nameFilters.first());
    }
//    qDebug() << "start_path name_filter" << start_path << name_filter;

    QFileDialog fileDialog(this, tr("Open Document"), start_path);
    fileDialog.setOptions(QFileDialog::DontUseNativeDialog);
    fileDialog.setNameFilters(SETTINGS.nameFilters);
    fileDialog.selectNameFilter(name_filter);

    if(fileDialog.exec() == QDialog::Accepted)
    {
        openFile(fileDialog.selectedFiles().constFirst());
        const QFileInfo fileInfo(fileDialog.selectedFiles().constFirst());
        SETTINGS.currentDir = fileInfo.dir();
//        SETTINGS.currentDir = fileInfo.absoluteDir();
    }
}

void NotPad::on_actionSave_triggered()
{
    qDebug() << "on_actionSave_triggered";
    save();
}

void NotPad::on_actionSave_as_triggered()
{
    qDebug() << "on_actionSave_as_triggered";
    saveAs();
}

void NotPad::on_actionAbout_triggered()
{
    qDebug() << "on_actionAbout_triggered";
    QString text = tr("A lightweight and small notepad application, "
                      "concentrating on plain text files. \n"
                      "\n"
                      "© %1 @ github\n"
                      ).arg(ORGANIZATION_NAME);

    QMessageBox::about(this, tr("About %1 v%2").arg(PROJECT_NAME, PROJECT_VERSION), text);
}

void NotPad::on_actionAboutQt_triggered()
{
    qDebug() << "on_actionAboutQt_triggered";
    QMessageBox::aboutQt(this);
}

void NotPad::on_find_findButton_clicked()
{
    QString searchString = ui->find_lineEdit->text();
    QTextDocument *document = m_editor->document();

    bool found = false;

    // undo previous change (if any)
    document->undo();

    if (searchString.isEmpty()) {
        QMessageBox::information(this, tr("Empty Search Field"),
                                 tr("The search field is empty. "
                                    "Please enter a word and click Find."));
    } else {
        QTextCursor highlightCursor(document);
        QTextCursor cursor(document);

        cursor.beginEditBlock();

        QTextCharFormat plainFormat(highlightCursor.charFormat());
        QTextCharFormat colorFormat = plainFormat;
//        colorFormat.setForeground(Qt::red);
        colorFormat.setBackground(Qt::yellow);

        while (!highlightCursor.isNull() && !highlightCursor.atEnd()) {
            highlightCursor = document->find(searchString, highlightCursor,
                                             QTextDocument::FindWholeWords);

            if (!highlightCursor.isNull()) {
                found = true;
                highlightCursor.movePosition(QTextCursor::WordRight,
                                             QTextCursor::KeepAnchor);
                highlightCursor.mergeCharFormat(colorFormat);
            }
        }

        cursor.endEditBlock();

        if (found == false) {
            QMessageBox::information(this, tr("Word Not Found"),
                                     tr("Sorry, the word cannot be found."));
        }
    }
}

void NotPad::on_actionWord_wrap_triggered(bool enabled)
{
//    qDebug() << "on_actionWord_wrap_triggered" << enabled;
    m_editor->setWordWrap(enabled);
    /// Latest choice by user is persisted
    SETTINGS.pers.wordWrap = enabled;
}

void NotPad::on_actionFontSmaller_triggered()
{
    SETTINGS.pers.zoomFontSize = incrementFontSize(-1);
}

void NotPad::on_actionFontLarger_triggered()
{
    SETTINGS.pers.zoomFontSize = incrementFontSize(1);
}

void NotPad::on_actionRestoreFontSize_triggered()
{
    SETTINGS.pers.zoomFontSize = restoreFontSize();
}

void NotPad::on_actionFind_triggered(bool checked)
{
    ui->main_find_widget->setHidden(!checked);
}

void NotPad::on_actionUndo_triggered()
{
    m_editor->undo();
}

void NotPad::on_actionRedo_triggered()
{
    m_editor->redo();
}

void NotPad::onUndoAvailable(bool available)
{
    qDebug() << "onUndoAvailable" << available << sender();
    ui->actionUndo->setEnabled(available);
}

void NotPad::onRedoAvailable(bool available)
{
    qDebug() << "onRedoAvailable" << available << sender();
    ui->actionRedo->setEnabled(available);
}

void NotPad::onTextChanged()
{
//    qDebug() << "onTextChanged" << sender();
}


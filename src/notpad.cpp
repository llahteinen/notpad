#include "notpad.hpp"
#include "forms/ui_notpad.h"
#include "tab.hpp"
#include "editor.hpp"
#include "statusbar.hpp"
#include "file.hpp"
#include "utils/utils.hpp"
#include "settings.hpp"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>
#include <QSettings>
#include <QThreadPool>
#include <QFuture>
#include <QToolButton>
#include <QStyleHints>



NotPad::NotPad(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NotPad)
    , m_locale{QLocale::system()}
    , m_statusBar{new StatusBar(m_locale, this)}
    , m_tabManager{}
    , m_editor{}
    , m_prevEditor{}
    , m_argumentFiles{}
{
    qInfo() << PROJECT_NAME << "starting";

    ui->setupUi(this);
    m_tabManager = ui->tabWidget;
    m_tabManager->setupUi();

    /// Create add tab button
    {
        auto* tb = new QToolButton(this);
        tb->setObjectName("menuBarPlusButton");
        tb->setIcon(QIcon::fromTheme("list-add"));
//        tb->setArrowType(Qt::ArrowType::DownArrow); /// Could be good for tab list button
        connect(tb, &QToolButton::clicked, m_tabManager, &TabManager::addEmptyTab);
        ui->menubar->setCornerWidget(tb, Qt::TopRightCorner);
    }

    setStatusBar(m_statusBar);

    /// Close the template tabs
    while(m_tabManager->count() > 0)
        m_tabManager->removeTab(0);

    connect(m_tabManager, &TabManager::currentChanged, this, &NotPad::onCurrentTabChanged);
    connect(m_tabManager, &TabManager::tabCloseRequested, this, &NotPad::onTabCloseRequested);

    connect(this, &NotPad::findResultFound, this, &NotPad::onFindResultFound);
    connect(this, &NotPad::matchCountFinished, this, &NotPad::onMatchCountFinished);

    QThreadPool::globalInstance()->setThreadPriority(QThread::HighPriority);

    QString project_name{PROJECT_NAME};
#if defined(QT_DEBUG)
    project_name.append("_dbg");
#endif
    setWindowTitle(QString("%1 v%2").arg(project_name, PROJECT_FULL_VERSION));

    QCoreApplication::setOrganizationName(ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(project_name);

    /// Check command line arguments
    handleArguments();

//    QApplication::setStyle("Fusion"); /// Similar look on all platforms, but not very native look on Windows. Does not seem to need much CSS/QSS customization
//    QApplication::setStyle("windows"); /// Vintage Windows style (but still supports dark mode!)
//    QApplication::setStyle("windowsvista"); /// Does not look super great. Does not support dark mode (but doesn't break either)
//    QApplication::setStyle("macOS"); /// Looks to fall back to windows11 style on Windows 11
//    QApplication::setStyle("windows11"); /// Good Windows 11 style (from Qt 6.11 onwards). Needs some CSS magic
    updateStyle(QGuiApplication::styleHints()->colorScheme());

    const auto* hints = QGuiApplication::styleHints();
    connect(hints, &QStyleHints::colorSchemeChanged, this, &NotPad::onColorSchemeChanged);

    on_actionFind_triggered(ui->actionFind->isChecked());

    qDebug() << "Platform:" << QGuiApplication::platformName();
    qDebug() << "Available XDG themes:" << QIcon::themeSearchPaths();
    qDebug() << "Current theme:" << QIcon::themeName();
    qDebug() << "Current style:" << QApplication::style();

    /// Load persisted data
    loadSettings(); /// Must be before QMainWindow::show() because it loads window size etc
    SETTINGS.pers.startupCounter++;
    qDebug() << "startups" << SETTINGS.pers.startupCounter;
}

NotPad::~NotPad()
{
    if(!QThreadPool::globalInstance()->waitForDone(1000))
    {
        qWarning() << "ThreadPool was not finished";
    }

    delete ui;

    if(!QThreadPool::globalInstance()->waitForDone(1000))
    {
        /// Abort
        qFatal() << "ThreadPool was still not finished, crashing...";
    }
}

StatusBar* NotPad::statusBar() const
{
    return m_statusBar;
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

/// Triggers always when window is restored from minimized to taskbar
/// What happens here happens just before the window is actually shown
void NotPad::showEvent(QShowEvent* event)
{
    qDebug() << "showEvent" << event << "spontaneous" << event->spontaneous();
}

void NotPad::show()
{
    qDebug() << "show";
    QMainWindow::show();
    qApp->processEvents(); /// Without this it starts with white screen

    /// Setup open tabs
    /// Load previous session
    /// TODO: restore the active tab?
    qDebug() << "sessionTabs" << SETTINGS.pers.sessionTabs;
    /// Check if the files exist?
    openFiles(SETTINGS.pers.sessionTabs);
    /// Argument files will be opened as last tabs
    openFiles(m_argumentFiles);

    /// Add empty tab if there is none
    if(m_tabManager->count() == 0) m_tabManager->addEmptyTab();

//    SETTINGS.currentDir = QDir("../../../testifiles"); /// Set save/load dialog starting location
}

void NotPad::dragEnterEvent(QDragEnterEvent* e)
{
    qDebug() << "dragEnterEvent";
    /// "A widget must accept this event in order to receive the drag move events"
    /// Must accept to show the OK cursor and for the possibly upcoming drop event to fire
    e->setAccepted(Utils::hasValidFiles(e->mimeData()));
}

void NotPad::dropEvent(QDropEvent* e)
{
    qDebug() << "dropEvent";
    const QStringList filenames = Utils::toFilelist(e->mimeData());
    for(auto& filename : filenames)
    {
        const auto status = m_tabManager->addTabFromFile(filename);
        messageOpenStatus(status);
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
}

void NotPad::handleArguments()
{
    /// arg0    = path to this executable
    /// arg1... = possible file
    /// Note that Qt automatically removes it's own supported args such as -widgetcount
    const auto arguments = qApp->arguments();
//    qDebug() << "args" << arguments;
    m_argumentFiles.clear();
    for(int i = 1; i < arguments.size(); ++i)
    {
        /// Check if we have files
        const QFileInfo arg{arguments.at(i)};
        if(arg.isFile())
        {
            qInfo() << "Got file argument" << arg.filePath();
            m_argumentFiles.append(arg.absoluteFilePath());
        }
    }
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
    /// Speed up shutdown a little bit if there are ongoing tasks running
    m_tabManager->iterateTabs([](Editor* editor) -> bool {
        Q_ASSERT(editor != nullptr);
        editor->abortTasks();
        return false;
    });

    /// 1. Save or discard modified tabs
    /// This closes tabs that were untitled and not saved,
    /// but does not close tabs that have a file even if they were not saved
    if(!cleanupModifiedTabs())
    {
        qDebug() << "closeAllTabs abort";
        return false;
    }

    /// 2. Persist remaining tabs, those that have a file whether saved or unsaved, as a session
    persistCurrentTabs();

    /// 3. Close all remaining tabs
    /// No need to ask permissions here anymore, since cleanupModifiedTabs already did
    while(m_tabManager->count())
    {
        /// Always close the active tab
        m_tabManager->closeCurrentTab();
        qApp->processEvents(); /// So that the UI briefly displays the new state before the whole window closes if this was the last tab
    }

    return true;
}

/// Closes tabs that don't have a file after asking user to save them (untitled discarded tabs)
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
            /// If the tab still does not have a file, means permission to close tab
            if(editor->file() == nullptr)
            {
                m_tabManager->closeTab(index);
            }
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
    ///  Discarded untitled will be closed, all that have a file will be left open
    /// Unmodified tabs will be left open and untouched
    bool success = false;
    m_tabManager->iterateTabs([this, &success](Editor* editor) -> bool {
        if(saveOrCloseTab(editor))
        {
            /// Update so that the UI briefly displays the new state before the whole window closes,
            /// if this was the last tab and the user saved it
            qApp->processEvents();
            success = true;
        }
        else
        {
            success = false; /// abort (Cancel pressed)
        }
        return !success; /// Return whether should break
    });

    return success;
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

/// Note that when opening tabs, this fires first and the file loading in the tab finishes later
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
    updateStatusBar();
}

void NotPad::setupSignals()
{
    qDebug() << "setupSignals" << m_prevEditor << m_editor;
    if(m_prevEditor)
    {
        disconnect(m_prevEditor, &QPlainTextEdit::undoAvailable, this, &NotPad::onUndoAvailable);
        disconnect(m_prevEditor, &QPlainTextEdit::redoAvailable, this, &NotPad::onRedoAvailable);
        disconnect(m_prevEditor, &QPlainTextEdit::textChanged,   this, &NotPad::onTextChanged);
        disconnect(m_prevEditor, &Editor::hasFileChanged,        this, &NotPad::onHasFileChanged);
    }
    if(m_editor)
    {
        /// Signals for active tab only (disconnect on every tab switch)
        connect(m_editor, &QPlainTextEdit::undoAvailable, this, &NotPad::onUndoAvailable, Qt::UniqueConnection);
        connect(m_editor, &QPlainTextEdit::redoAvailable, this, &NotPad::onRedoAvailable, Qt::UniqueConnection);
        connect(m_editor, &QPlainTextEdit::textChanged,   this, &NotPad::onTextChanged,   Qt::UniqueConnection);
        connect(m_editor, &Editor::hasFileChanged,        this, &NotPad::onHasFileChanged, Qt::UniqueConnection);
        connect(m_editor, &QPlainTextEdit::cursorPositionChanged, this, &NotPad::onCursorPositionChanged, Qt::UniqueConnection);

        /// Signals for active and background tabs, not supposed to get disconnected on tab switch
        connect(m_editor, &Editor::dataLoadingFinished,   this, &NotPad::onLoadingFinished, Qt::UniqueConnection);
        connect(m_editor, &Editor::dataLoadingUpdate,     this, &NotPad::onLoadingUpdate, Qt::UniqueConnection);
        connect(this, &NotPad::findBoxVisibleChanged, m_editor, &Editor::setHighlighterEnabled, Qt::UniqueConnection);
    }
}

void NotPad::setupMenu()
{
    if(m_editor)
    {
        onUndoAvailable(m_editor->document()->isUndoAvailable());
        onRedoAvailable(m_editor->document()->isRedoAvailable());
        onHasFileChanged(m_editor->file() != nullptr);
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

void NotPad::updateStatusBar()
{
    if(m_editor)
    {
        StatusBarData sbdata;
        sbdata.encoding = { m_editor->encodingName(), m_editor->endOfLineName() };
        sbdata.stats = { m_editor->document()->blockCount(), m_editor->document()->characterCount() };
        sbdata.cursor = { m_editor->textCursor() };
        statusBar()->update(sbdata);
    }
}

void NotPad::updateStyle(Qt::ColorScheme scheme)
{
    /// Set colors based on OS dark/light mode
    qDebug() << "Color scheme" << scheme;
    QFile colorStyleFile;
    if(scheme == Qt::ColorScheme::Dark)
    {
        colorStyleFile.setFileName(":/forms/colors-dark.css");
    }
    else
    {
        colorStyleFile.setFileName(":/forms/colors-light.css");
    }

    if(colorStyleFile.open(QFile::ReadOnly))
    {
        const auto style = colorStyleFile.readAll();
        qApp->setStyleSheet(style);
        if(style.trimmed().isEmpty())
        {
            qWarning() << "Style was empty.";
        }
    }
    else
    {
        qWarning() << "Setting color style failed.";
    }

    /// Set other style settings common for all color themes
    if(QFile styleFile(":/forms/styles.css"); styleFile.open(QFile::ReadOnly))
    {
        const auto style = styleFile.readAll();
        this->setStyleSheet(style);
        if(style.trimmed().isEmpty())
        {
            qWarning() << "Style was empty.";
        }
    }
    else
    {
        qWarning() << "Setting common style failed.";
    }
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
    if(status != File::Status::SUCCESS_READ)
    {
        messageOpenStatus(status);
    }
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

bool NotPad::confirmFileReload(Editor* editor, const QString& messageTitle)
{
    qDebug() << "NotPad::confirmFileReload";
    bool permission = false;
    if(editor->isModified())
    {
        qDebug() << "File is edited";
        const auto choice = QMessageBox::warning(this, messageTitle,
                                                 tr("The document has been modified.\n"
                                                    "Do you want to discard the changes\n"
                                                    "and reload from disk?"),
                                                 QMessageBox::Discard | QMessageBox::Cancel,
                                                 QMessageBox::Cancel);

        qDebug() << "Choice" << choice;
        switch(choice)
        {
        case QMessageBox::Discard:
            {
                permission = true;
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

/// EVENT HANDLERS =======================================

void NotPad::keyPressEvent(QKeyEvent* event)
{
//    qDebug() << "keyPressEvent" << event;
    switch(event->key())
    {
    case Qt::Key_Escape:
        {
//            qDebug() << "Escape";
            if(ui->main_find_widget->isVisible())
            {
                /// First press pops focus off find box,
                /// second press hides find ui
                if(ui->find_lineEdit->hasFocus() && m_editor)
                {
                    m_editor->setFocus(Qt::OtherFocusReason);
                }
                else
                {
                    on_actionFind_triggered(false);
                }
                return;
            }
            break;
        }
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_F3:
        {
            /// F3 works always, enter only when find is active
            if(event->key() != Qt::Key_F3 && ui->main_find_widget->isHidden())
            {
                break;
            }
            const auto has_shift = event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier);
            const auto is_repeat = event->isAutoRepeat();
            const auto btn = has_shift ? ui->find_findPrevButton : ui->find_findButton;
            is_repeat ? btn->click() : btn->animateClick();
            return;
        }
    case Qt::Key_W:
        {
            const auto has_ctrl = event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier);
            const auto is_repeat = event->isAutoRepeat();
            if(has_ctrl && !is_repeat && m_editor) // && (m_editor->hasFocus() || m_tabManager->hasFocus())) need to think carefully do we want to check focus here
            {
                onTabCloseRequested(m_tabManager->currentIndex());
                return;
            }
            break;
        }
    default:
        break;
    }
    /// Call the base class implementation if we did NOT handle the event
    QMainWindow::keyPressEvent(event);
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
    if(m_editor && m_editor->file() != nullptr)
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

void NotPad::on_actionReload_from_disk_triggered()
{
    Editor* editor = m_editor;
    Q_ASSERT(editor != nullptr);
    if(editor != nullptr)
    {
        if(confirmFileReload(editor, editor->name()))
        {
            editor->reload();
        }
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
    find({});
}
void NotPad::on_find_findPrevButton_clicked()
{
    find(QTextDocument::FindFlag::FindBackward);
}

void NotPad::onFindResultFound(Editor* editor, QTextCursor result)
{
    if(!editor || editor != m_editor)
    {
        /// Weird situation
        return;
    }
    editor->setTextCursor(result);
}

void NotPad::onMatchCountFinished(Editor* editor, qsizetype count)
{
    if(!editor || editor != m_editor)
    {
        /// Weird situation
        return;
    }

    qInfo() << "Matches count" << count;
    statusBar()->showMessage(tr("%1 matches").arg(count));
    if(count <= 0)
    {
        QApplication::beep();
    }
}

void NotPad::find(QTextDocument::FindFlags flags, int recursion)
{
//    qDebug() << "find" << flags;
    qDebug() << "recursion" << recursion;
    const QString& searchString = ui->find_lineEdit->text();
    const QTextDocument* document = m_editor->document();

    if(searchString.isEmpty())
    {
        statusBar()->showMessage(tr("Empty Search Field"), 1000);
    }
    else
    {
        /// First start counting the results because it can take a while and can be done in background
        const auto count_promise = std::make_shared<QPromise<qsizetype>>(); /// shared ptr because we might return early putting promise out of scope
        auto count_future = count_promise->future();
        if(!recursion)
        {
            /// Capture by value because we might return early
            QThreadPool::globalInstance()->start([=]{
                count_promise->start();
                const auto count = m_editor->getMatchCount(searchString, flags);
                count_promise->addResult(count);
                count_promise->finish();
                emit matchCountFinished(m_editor, count);
            });
        }

        /// Find the next result whether we are recursed or not
        const QTextCursor result = document->find(searchString, m_editor->textCursor(), flags);
        qDebug() << "result.isNull" << result.isNull();

        /// Found a result, jump and start hilighting
        if(!result.isNull())
        {
            emit findResultFound(m_editor, result);

            m_editor->setHighlightRegex(searchString, flags);

            return;
        }

        /// Result not found
        if(!recursion)
        {
            /// Wait for result count to see if we should try wrapping around the document
            count_future.waitForFinished();
            if(count_future.result() <= 0)
            {
                /// Should we unhilight previous matches if the new search didn't find anything?
                /// np++ doesn't. But it unhilights immediately when focus goes back to editor (if selection changes)
//                m_editor->highLighter->clear();
                return; /// Don't jump anywhere, nothing was found in the whole document
            }

            /// Not found, but result count > 0
            /// so let's wrap around the document
//            qDebug() << "atEnd" << result.atEnd();
//            qDebug() << "atStart" << result.atStart();
            if(!flags.testFlag(QTextDocument::FindFlag::FindBackward))
            {
//                qDebug() << "FindForward";
                m_editor->moveCursor(QTextCursor::Start);
                qInfo() << "Find jumped to start";
                statusBar()->showMessage(tr("Jumped to start"), 1000);
                find(flags, ++recursion);
            }
            else if(flags.testFlag(QTextDocument::FindFlag::FindBackward))
            {
//                qDebug() << "FindBackward";
                m_editor->moveCursor(QTextCursor::End);
                qInfo() << "Find jumped to end";
                statusBar()->showMessage(tr("Jumped to end"), 1000);
                find(flags, ++recursion);
            }
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
    m_editor->incrementFontSize(-1);
}

void NotPad::on_actionFontLarger_triggered()
{
    m_editor->incrementFontSize(1);
}

void NotPad::on_actionRestoreFontSize_triggered()
{
    m_editor->restoreFontSize();
}

void NotPad::on_actionFind_triggered(bool checked)
{
    qDebug() << "on_actionFind_triggered" << checked;
    if(!m_editor)
    {
        ui->actionFind->setChecked(checked);
        ui->main_find_widget->setVisible(checked);
        return;
    }

    /// Three scenarios:
    /// Find widget is not currently visible
    ///  -> show
    /// Find widget is already visible, but the selected text is different than what is in the search input box
    ///  -> set new search text
    /// Find widget is already visible and the selected text has stayed the same
    ///  -> hide

    bool show = checked;
    if(m_editor->textCursor().hasSelection()) /// hasComplexSelection ?
    {
        qDebug() << "hasSelection" << m_editor->textCursor().selectedText();
        const auto selected_text = m_editor->textCursor().selectedText();
        if(selected_text.compare(ui->find_lineEdit->text(), Qt::CaseInsensitive) != 0) /// Should match search setting?
        {
            /// User has selected something else, probably wants to search for it and not close the search
            show = true;
            ui->find_lineEdit->setText(selected_text);
        }
    }

    const bool raising_edge = !ui->main_find_widget->isVisible() && show;
    const bool falling_edge = ui->main_find_widget->isVisible() && !show;

    ui->actionFind->setChecked(show);
    ui->main_find_widget->setVisible(show);
    if(show)
    {
        ui->find_lineEdit->setFocus(Qt::OtherFocusReason);
        ui->find_lineEdit->selectAll();
    }
    else
    {
        m_editor->setFocus(Qt::OtherFocusReason);
        statusBar()->clearMessage(); /// Maybe we should use a dedicated text info box for search
    }

    if(raising_edge || falling_edge)
    {
        emit findBoxVisibleChanged(show);
    }
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

void NotPad::onHasFileChanged(bool has)
{
    qDebug() << "onHasFileChanged" << has;
    ui->actionReload_from_disk->setEnabled(has);
}

void NotPad::onTextChanged()
{
//    qDebug() << "onTextChanged" << sender();
}

void NotPad::onCursorPositionChanged()
{
//    qDebug() << "onCursorPositionChanged";
    if(!m_editor)
    {
        return;
    }
    StatusBarData sbdata;
    sbdata.cursor = { m_editor->textCursor() };
    statusBar()->update(sbdata);
}

void NotPad::onLoadingUpdate(int progress)
{
//    qDebug() << "onLoadingUpdate";
    /// This could also be a progress bar in the statusbar
    if(sender() == m_editor)
    {
        StatusBarData sbdata;
        sbdata.stats = { m_editor->document()->blockCount(), progress };
        statusBar()->update(sbdata);
    }
}

void NotPad::onLoadingFinished()
{
    qDebug() << "onLoadingFinished" << sender();
    const auto* editor = qobject_cast<Editor*>(sender());
    if(editor)
    {
        statusBar()->showMessage(tr("Finished loading %1").arg(editor->name()), 2000);
    }
    updateStatusBar();
}

void NotPad::onColorSchemeChanged(Qt::ColorScheme colorScheme)
{
    qDebug() << "onColorSchemeChanged" << colorScheme;
    /// Must be queued call, otherwise the styles get mangled badly.
    /// Seems that this colorSchemeChanged signal is emitted before the theme has actually changed.
    /// "When the colorSchemeChange() signal gets emitted, the old palette is still in effect."
    /// "To update application- specific colors when the effective palette changes, handle PaletteChange or ApplicationPaletteChange events."
    QMetaObject::invokeMethod(this, &NotPad::updateStyle, Qt::QueuedConnection, colorScheme);
}


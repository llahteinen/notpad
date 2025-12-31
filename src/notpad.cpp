#include "notpad.hpp"
#include "forms/ui_notpad.h"
#include "tab.hpp"
#include "editor.hpp"
#include "file.hpp"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>
#include <QCloseEvent>



NotPad::NotPad(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NotPad)
    , m_currentDir{}
    , m_settings{}
{
    qInfo() << PROJECT_NAME << "starting";

    ui->setupUi(this);
    m_tabManager = new TabManager(ui->tabWidget, ui->textEdit, this); /// ui->textEdit is used as template for all new tabs
    /// Close the template tabs
    while(ui->tabWidget->count() > 0)
        ui->tabWidget->removeTab(0);

    connect(m_tabManager, &TabManager::currentChanged, this, &NotPad::onCurrentTabChanged);
    connect(m_tabManager, &TabManager::tabCloseRequested, this, &NotPad::onTabCloseRequested);

    setWindowTitle(QString("%1 v%2").arg(PROJECT_NAME, PROJECT_VERSION));

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

    m_tabManager->addEmptyTab();
    setupEditor();

//    /// Open a test text file
//    openFile(":/forms/input.txt"); /// Breaks m_currentDir
//    m_file.reset(); /// :/forms does not work well, so reset it
//    m_currentDir = QDir("../../../testifiles");
}

NotPad::~NotPad()
{
    delete ui;
}

void NotPad::closeEvent(QCloseEvent* event)
{
    qDebug() << "MainWindow closeEvent";
    if(!SETTINGS.confirmAppClose || confirmAppClose(tr("Quitting")))
    {
        QMainWindow::closeEvent(event);
    }
    else
    {
        /// Don't close
        event->ignore();
    }
}

void NotPad::onTabCloseRequested(int index)
{
    auto* widget = ui->tabWidget->widget(index);
    qDebug() << "widget" << widget;
    auto* editor = qobject_cast<Editor*>(widget);
    Q_ASSERT(editor != nullptr);
    if(editor != nullptr)
    {
        if(confirmFileClose(editor, tr("Closing tab")))
        {
            m_tabManager->closeTab(index);
        }
        else
        {
            /// Don't close
        }
    }
}

void NotPad::onCurrentTabChanged(int index)
{
    qDebug() << "onCurrentTabChanged" << index;
    /// Last tab closed
    if(index == -1)
    {
        m_editor = nullptr;

        /// Close application after last tab?
        if(true)
            qApp->quit();
        return;
    }

    auto* widget = m_tabManager->currentWidget();
    qDebug() << "widget" << widget;
    auto* editor = qobject_cast<Editor*>(widget);
    Q_ASSERT(editor != nullptr);
    if(editor != nullptr)
    {
        m_editor = editor;
    }
    else
    {
        qDebug() << "Invalid editor" << editor;
    }
    qDebug() << "file" << currentFile();
}

void NotPad::setupEditor()
{
    connect(m_editor, &QPlainTextEdit::undoAvailable, this, &NotPad::onUndoAvailable);
    connect(m_editor, &QPlainTextEdit::redoAvailable, this, &NotPad::onRedoAvailable);
    connect(m_editor, &QPlainTextEdit::textChanged,   this, &NotPad::onTextChanged);

    restoreFontSize();
}

void NotPad::incrementFontSize(int increment)
{
    auto font = m_editor->font();
    auto size = font.pointSize();
    auto index = m_settings.standardFontSizes.indexOf(size);
    Q_ASSERT(index >= 0);
    if(index < 0)
    {
        qWarning() << "Got weird font size" << size;
        index = m_settings.standardFontSizes.indexOf(m_settings.fontSizeDefault);
    }
    index += increment;
    index = qMax(index, 0);
    index = qMin(index, m_settings.standardFontSizes.length()-1);
    qDebug() << "index" << index;
    size = m_settings.standardFontSizes.at(index);
    qDebug() << "size" << size;
    font.setPointSize(size);
    m_editor->setFont(font);
    updateTabWidth(); /// TODO: Implement using QEvent::FontChange
}

void NotPad::restoreFontSize()
{
    auto font = m_editor->font();
    font.setPointSize(m_settings.fontSizeDefault);
    qDebug() << "pointSize" << font.pointSize();
    m_editor->setFont(font);
    updateTabWidth();
}

void NotPad::updateTabWidth()
{
    m_editor->setTabStopDistance(m_settings.tabWidthChars * m_editor->fontMetrics().averageCharWidth());
}

bool NotPad::openFile(const QString &fileName)
{
    auto file = std::make_unique<QFile>(fileName);
    if(!file->exists())
    {
        statusBar()->showMessage(tr("File %1 could not be opened")
                                     .arg(QDir::toNativeSeparators(fileName)));
        return false;
    }
    Q_ASSERT(file);
    m_editor->m_file = std::move(file);

    QFileInfo fileInfo(*currentFile());
    m_currentDir = fileInfo.dir();
//    m_currentDir = fileInfo.absoluteDir();

    statusBar()->showMessage(tr("File opened: %1").arg(QDir::toNativeSeparators(fileName)));

    if(!currentFile()->open(QFile::ReadOnly | QFile::Text))
    {
        statusBar()->showMessage(tr("Cannot read file %1:\n%2.").arg(QDir::toNativeSeparators(fileName), currentFile()->errorString()));
        return false;
    }

    QTextStream fileStream(currentFile());
    m_editor->setPlainText(fileStream.readAll());
    currentFile()->close(); /// Free the file resource for use by other processes

    /// setText clears undo history, but the undo/redo available signals might not be emitted
    onUndoAvailable(false);
    onRedoAvailable(false);
    m_editor->document()->setModified(false);
    return true;
}

void NotPad::messageSaveStatus(File::Status status, const QFile* const file)
{
    QString msg;
    switch(status)
    {
    case File::Status::CANCELED:
        break;
    case File::Status::FAIL_OPEN_WRITE:
        msg = tr("Cannot open file for writing: %1").arg(file->errorString());
        break;
    case File::Status::FAIL_WRITE:
        msg = tr("File write failed: %1").arg(file->errorString());
        break;
    case File::Status::FAIL_WRITE_UNKNOWN:
        msg = tr("File write failed: %1").arg(file->errorString());
        break;
    case File::Status::SUCCESS_WRITE:
        msg = tr("File saved: %1").arg(QDir::toNativeSeparators(file->fileName()));
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
    const auto status = editor->save();
    messageSaveStatus(status, editor->m_file.get());
    return status == File::Status::SUCCESS_WRITE;
}

bool NotPad::saveAs()
{
    const auto status = m_editor->saveAs();
    messageSaveStatus(status, m_editor->m_file.get());
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

QFile* NotPad::currentFile()
{
    if(m_editor != nullptr)
    {
        return m_editor->m_file.get();
    }
    return nullptr;
}

/// SLOTS ================================================

void NotPad::on_actionNew_triggered()
{
    qDebug() << "on_actionNew_triggered";
    if(confirmFileClose(m_editor, tr("New file")))
    {
        m_editor->clear();
        m_editor->m_file.reset();
        m_editor->document()->setModified(false);
    }
}

void NotPad::on_actionOpen_triggered()
{
    qDebug() << "on_actionOpen_triggered";
    QFileDialog fileDialog(this, tr("Open Document"), m_currentDir.absolutePath());
    fileDialog.setOptions(QFileDialog::DontUseNativeDialog);
    fileDialog.setNameFilters(SETTINGS.nameFilters);
    while (fileDialog.exec() == QDialog::Accepted
           && !openFile(fileDialog.selectedFiles().constFirst())) {
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
                      "concentrating on plain text files. ");

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
    qDebug() << "on_actionWord_wrap_triggered" << enabled;
    const auto wrap_mode = enabled ? QTextOption::WrapMode::WordWrap
                                   : QTextOption::WrapMode::NoWrap;
    /// NOTE: maybe with binary files could use WrapAnywhere
    m_editor->setWordWrapMode(wrap_mode);
}

void NotPad::on_actionFontSmaller_triggered()
{
    incrementFontSize(-1);
}

void NotPad::on_actionFontLarger_triggered()
{
    incrementFontSize(1);
}

void NotPad::on_actionRestoreFontSize_triggered()
{
    restoreFontSize();
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
    qDebug() << "onUndoAvailable" << available;
    ui->actionUndo->setEnabled(available);
}

void NotPad::onRedoAvailable(bool available)
{
    qDebug() << "onRedoAvailable" << available;
    ui->actionRedo->setEnabled(available);
}

void NotPad::onTextChanged()
{
//    qDebug() << "onTextChanged";
}


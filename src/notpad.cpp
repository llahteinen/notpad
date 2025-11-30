#include "notpad.hpp"
#include "forms/ui_notpad.h"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QFileDialog>



NotPad::NotPad(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NotPad)
    , m_defaultNameFilter{m_nameFilters.at(1)}
    , m_currentNameFilter{m_defaultNameFilter}
    , m_currentDir{}
    , m_file{}
    , m_fileEdited{}
{
    qInfo() << PROJECT_NAME << "starting";

    ui->setupUi(this);
    m_editor = ui->textEdit;

    setWindowTitle(QString("%1 v%2").arg(PROJECT_NAME, PROJECT_VERSION));


    connect(m_editor, &QTextEdit::undoAvailable, this, &NotPad::onUndoAvailable);
    connect(m_editor, &QTextEdit::redoAvailable, this, &NotPad::onRedoAvailable);
    connect(m_editor, &QTextEdit::textChanged,   this, &NotPad::onTextChanged);


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

    /// Open a test text file
    openFile(":/forms/input.txt"); /// Breaks m_currentDir
    m_file.reset(); /// :/forms does not work well, so reset it
    m_currentDir = QDir("../../../testifiles");
}

NotPad::~NotPad()
{
    delete ui;
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
    m_file = std::move(file);

    QFileInfo fileInfo(*m_file);
    m_currentDir = fileInfo.dir();
//    m_currentDir = fileInfo.absoluteDir();

    statusBar()->showMessage(tr("File opened: %1").arg(QDir::toNativeSeparators(fileName)));

    if(!m_file->open(QFile::ReadOnly | QFile::Text))
    {
        statusBar()->showMessage(tr("Cannot read file %1:\n%2.").arg(QDir::toNativeSeparators(fileName), m_file->errorString()));
        return false;
    }

    QTextStream fileStream(m_file.get());
    m_editor->setText(fileStream.readAll());
    m_file->close(); /// Free the file resource for use by other processes

    /// setText clears undo history, but the undo/redo available signals might not be emitted
    onUndoAvailable(false);
    onRedoAvailable(false);
    m_fileEdited = false;
    return true;
}

bool NotPad::saveFile(QFile* const file)
{
    Q_ASSERT(file);
    if(file->exists())
    {
        qDebug() << "About to overwrite" << m_file->fileName();
    }
    else
    {
        qDebug() << "Writing new file" << m_file->fileName();
    }
    const QFileInfo fileInfo(*file);
    m_currentDir = fileInfo.dir();

    if(!file->open(QFile::WriteOnly | QFile::Text))
    {
        qWarning() << "Failed to open file:" << file->errorString();
        statusBar()->showMessage(tr("Cannot open file for writing %1:\n%2.").arg(QDir::toNativeSeparators(file->fileName()), file->errorString()));
        return false;
    }

    QTextStream fstream{file};
    fstream.setEncoding(QStringConverter::Utf8);
    fstream << m_editor->toPlainText();
    if(fstream.status() != QTextStream::Ok)
    {
        qWarning() << "Failed to write to file:" << fstream.status();
        statusBar()->showMessage(tr("File write failed %1:\n%2.").arg(fstream.status()));
        file->close();
        return false;
    }

    fstream.flush(); // Ensure data is written to the file
    if(file->error() != QFile::NoError)
    {
        qWarning() << "File error after writing:" << file->errorString();
        statusBar()->showMessage(tr("File write failed %1:\n%2.").arg(file->errorString()));
        file->close();
        return false;
    }

    file->close(); /// Free the file resource for use by other processes
    statusBar()->showMessage(tr("File saved: %1").arg(QDir::toNativeSeparators(file->fileName())));
    return true;
}

bool NotPad::saveFile(const QString &fileName)
{
    auto file = std::make_unique<QFile>(fileName);
    Q_ASSERT(file);
    m_file = std::move(file);
    return saveFile(m_file.get());
}

bool NotPad::save()
{
    bool saved = false;
    if(!m_file)
    {
        saved = saveAs();
    }
    else
    {
        saved = saveFile(m_file.get());
    }

    if(saved)
    {
        m_fileEdited = false;
    }
    return saved;
}

bool NotPad::saveAs()
{
    QFileDialog fileDialog(this, tr("Save Document"), m_currentDir.absolutePath());
    fileDialog.setOptions(QFileDialog::DontUseNativeDialog);
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setViewMode(QFileDialog::ViewMode::Detail);
//    fileDialog.setDefaultSuffix("txt");   /// TODO: Make selectable?
    fileDialog.setFileMode(QFileDialog::FileMode::AnyFile);
    fileDialog.setNameFilters(m_nameFilters);
    fileDialog.selectNameFilter(m_currentNameFilter);

    bool saved = false;
    if(fileDialog.exec() == QDialog::Accepted)
    {
        m_currentNameFilter = fileDialog.selectedNameFilter();

        qDebug() << fileDialog.selectedFiles();
        Q_ASSERT(fileDialog.selectedFiles().size() == 1 && "Selected save file count must be 1");
        saved = saveFile(fileDialog.selectedFiles().at(0));
    }

    if(saved)
    {
        m_fileEdited = false;
    }
    return saved;
}


/// SLOTS ================================================

void NotPad::on_actionNew_triggered()
{
    qDebug() << "on_actionNew_triggered";
    if(m_fileEdited)
    {
        qDebug() << "File is edited";
        const auto choice = QMessageBox::warning(this, tr("New file"),
                                       tr("The document has been modified.\n"
                                          "Do you want to save your changes?"),
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                       QMessageBox::Save);

        qDebug() << "Choice" << choice;
        switch(choice)
        {
        case QMessageBox::Discard:
            {
                break;
            }
        case QMessageBox::Save:
            {
                if(save()) break;   /// File was saved
                else return;        /// File saving canceled or failed
            }
        case QMessageBox::Cancel:
        default:
            {
                return;
            }
        }
    }

    m_editor->clear();
    m_file.reset();
    m_fileEdited = false;
}

void NotPad::on_actionOpen_triggered()
{
    qDebug() << "on_actionOpen_triggered";
    QFileDialog fileDialog(this, tr("Open Document"), m_currentDir.absolutePath());
    fileDialog.setOptions(QFileDialog::DontUseNativeDialog);
    fileDialog.setNameFilters(m_nameFilters);
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
    m_fileEdited = true;
}


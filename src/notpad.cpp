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
{
    qInfo() << PROJECT_NAME << "starting";

    ui->setupUi(this);
    m_editor = ui->textEdit;

    setWindowTitle(QString("%1 v%2").arg(PROJECT_NAME, PROJECT_VERSION));


    connect(m_editor, &QTextEdit::undoAvailable, this, &NotPad::onUndoAvailable);
    connect(m_editor, &QTextEdit::redoAvailable, this, &NotPad::onRedoAvailable);


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
    return true;
}


/// SLOTS ================================================

void NotPad::on_actionOpen_triggered()
{
    qDebug() << "on_actionOpen_triggered";
    QFileDialog fileDialog(this, tr("Open Document"), m_currentDir.absolutePath());
    fileDialog.setOptions(QFileDialog::DontUseNativeDialog);
    while (fileDialog.exec() == QDialog::Accepted
           && !openFile(fileDialog.selectedFiles().constFirst())) {
    }
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


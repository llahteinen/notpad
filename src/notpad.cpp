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

    qDebug() << "Platform:" << QGuiApplication::platformName();
    qDebug() << "Available XDG themes:" << QIcon::themeSearchPaths();
    qDebug() << "Current theme:" << QIcon::themeName();

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
    ui->textEdit->setText(fileStream.readAll());
    m_file->close(); /// Free the file resource for use by other processes
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

void NotPad::on_findButton_clicked()
{
    QString searchString = ui->lineEdit->text();
    QTextDocument *document = ui->textEdit->document();

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

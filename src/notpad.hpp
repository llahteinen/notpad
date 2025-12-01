#ifndef NOTPAD_HPP
#define NOTPAD_HPP

#include "Settings.hpp"
#include <QMainWindow>
#include <QDir>

class QPlainTextEdit;

QT_BEGIN_NAMESPACE
namespace Ui {
class NotPad;
}
QT_END_NAMESPACE


class NotPad : public QMainWindow
{
    Q_OBJECT

public:
    NotPad(QWidget *parent = nullptr);
    ~NotPad();

private:
    void closeEvent(QCloseEvent* event) override;

    void setupEditor();

    void incrementFontSize(int increment);
    void restoreFontSize();
    void updateTabWidth();

    bool openFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    bool saveFile(QFile* file);
    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    bool save();
    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    bool saveAs();
    /// \return true for permission to close file, false for no permission
    bool confirmFileClose(const QString& messageTitle = tr("Confirmation"));

private slots:
    /// Custom slots
    void onUndoAvailable(bool available);
    void onRedoAvailable(bool available);
    void onTextChanged();

    /// Automatically connected slots
    /// MENU ================================
    /// File menu
    void on_actionNew_triggered();
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_as_triggered();
    /// Edit menu
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionFind_triggered(bool checked);
    /// Help menu
    void on_actionAbout_triggered();
    void on_actionAboutQt_triggered();
    /// Options menu
    void on_actionWord_wrap_triggered(bool enabled);
    void on_actionFontSmaller_triggered();
    void on_actionFontLarger_triggered();
    void on_actionRestoreFontSize_triggered();
    /// /MENU ================================

    void on_find_findButton_clicked();

private:

    Ui::NotPad *ui;

    QStringList m_nameFilters
    {
        "All files (*)",
        "Text files (*.txt)",
        "Log files (*.log)",
        "Markdown files (*.md)",
        "JSON files (*.json)",
        "C/C++ files (*.cpp *.hpp *.c *.h)",
        "HTML files (*.htm *.html *.php)",
        "CSS files (*.css)",
    };
//    QStringList m_mimeTypeFilters{ /// This is alternative to nameFilters, both can't be used together
//        "text/plain", /// Returns a huge amount of suffixes
//        "text/csv",
//        "text/html",
//        "application/json",
//        "application/octet-stream" /// will show "All files (*)"
//    };
    const QString m_defaultNameFilter;
    QString m_currentNameFilter;
    QDir m_currentDir;
    std::unique_ptr<QFile> m_file;
    bool m_fileEdited;

    Settings m_settings;

    QPlainTextEdit* m_editor;
};


#endif // NOTPAD_HPP

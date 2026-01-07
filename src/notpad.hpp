#ifndef NOTPAD_HPP
#define NOTPAD_HPP

#include "settings.hpp"
#include "file.hpp"
#include <QMainWindow>
#include <QDir>

class TabManager;
class Editor;
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

    bool closeAllTabs();

    void setupSignals();
    void setupMenu();

    void incrementFontSize(int increment);
    void restoreFontSize();
    void updateTabWidth();

    void messageOpenStatus(const File::Status& status);
    void messageSaveStatus(const File::Status& status);

    bool openFile(const QString &fileName);
    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    bool save();
    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    bool save(Editor* editor);
    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    bool saveAs();
    /// \return true for permission to close application, false for no permission
    bool confirmAppClose(const QString& messageTitle = tr("Confirmation"));
    /// \return true for permission to close file, false for no permission
    bool confirmFileClose(Editor* editor, const QString& messageTitle = tr("Confirmation"));

    QFile* currentFile();

private slots:
    /// Custom slots
    void onUndoAvailable(bool available);
    void onRedoAvailable(bool available);
    void onTextChanged();
    void onCurrentTabChanged(int index);
    bool onTabCloseRequested(int index);

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

    Settings m_settings;

    TabManager* m_tabManager;
    Editor* m_editor;       //!< Editor that is currently selected in the active tab
    Editor* m_prevEditor;   //!< Editor that was selected before the current one
};


#endif // NOTPAD_HPP

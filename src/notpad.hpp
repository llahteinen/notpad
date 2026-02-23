#ifndef NOTPAD_HPP
#define NOTPAD_HPP

#include "file.hpp"
#include <QMainWindow>
#include <QTextDocument>

class TabManager;
class Editor;
class QLabel;

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
    NotPad(const NotPad&) = delete;
    NotPad& operator=(const NotPad&) = delete;

public slots:
    void show();    /// Hide base class show()

private:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

    void saveSettings();
    void loadSettings();
    void handleArguments();

    void persistCurrentTabs();
    bool closeAllTabs();
    bool saveOrCloseTab(Editor* editor);
    bool cleanupModifiedTabs();

    void setupSignals();
    void setupMenu();
    void setupStatusBar();

    int incrementFontSize(int increment);
    int restoreFontSize();
    void updateTabWidth();

    void messageOpenStatus(const File::Status& status);
    void messageSaveStatus(const File::Status& status);

    void openFiles(const QStringList &fileNameList);
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

    void find(QTextDocument::FindFlags flags, int recursion = 0);

    const QFile* currentFile();

private slots:
    /// Custom slots
    void onUndoAvailable(bool available);
    void onRedoAvailable(bool available);
    void onTextChanged();
    void onLoadingUpdate(int progress);
    void onLoadingFinished();
    void onCurrentTabChanged(int index);
    bool onTabCloseRequested(int index);
    void onFindResultFound(Editor* editor, QTextCursor result);
    void onMatchCountFinished(Editor* editor, qsizetype count);

    /// Automatically connected slots
    /// MENU ================================
    /// File menu
    void on_actionNew_triggered();
    void on_actionNewTab_triggered();
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

    /// FIND ================================
    void on_find_findButton_clicked();
    void on_find_findPrevButton_clicked();
    /// /FIND ================================

private:
    void keyPressEvent(QKeyEvent* event) override;

    Ui::NotPad *ui;

    QLabel* m_statusEncodingLabel;  //!< The rightmost text box in status bar

    TabManager* m_tabManager;
    Editor* m_editor;       //!< Editor that is currently selected in the active tab
    Editor* m_prevEditor;   //!< Editor that was selected before the current one

    QStringList m_argumentFiles;

signals:
    void findResultFound(Editor* editor, QTextCursor result);
    void matchCountFinished(Editor* editor, qsizetype count);

};


#endif // NOTPAD_HPP

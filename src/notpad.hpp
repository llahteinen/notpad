#ifndef NOTPAD_HPP
#define NOTPAD_HPP

#include <QMainWindow>
#include <QDir>

class QTextEdit;

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
    bool openFile(const QString &fileName);

private slots:
    /// Custom slots
    void onUndoAvailable(bool available);
    void onRedoAvailable(bool available);

    /// MENU ================================
    /// File menu
    void on_actionOpen_triggered();
    /// Edit menu
    void on_actionUndo_triggered();
    void on_actionRedo_triggered();
    void on_actionFind_triggered(bool checked);
    /// Help menu
    void on_actionAbout_triggered();
    void on_actionAboutQt_triggered();
    /// Options menu
    void on_actionWord_wrap_triggered(bool enabled);
    /// /MENU ================================

    void on_find_findButton_clicked();

private:

    QDir m_currentDir;
    std::unique_ptr<QFile> m_file;

    QTextEdit* m_editor;
    Ui::NotPad *ui;
};


#endif // NOTPAD_HPP

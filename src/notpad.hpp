#ifndef NOTPAD_HPP
#define NOTPAD_HPP

#include <QMainWindow>
#include <QDir>


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
    void on_actionOpen_triggered();
    void on_actionAbout_triggered();
    void on_actionAboutQt_triggered();

    void on_findButton_clicked();

private:

    QDir m_currentDir;
    std::unique_ptr<QFile> m_file;

    Ui::NotPad *ui;
};


#endif // NOTPAD_HPP

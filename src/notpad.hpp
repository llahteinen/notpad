#ifndef NOTPAD_HPP
#define NOTPAD_HPP

#include <QMainWindow>

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
    Ui::NotPad *ui;
};
#endif // NOTPAD_HPP

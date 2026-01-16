#include "notpad.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    NotPad w;
    w.setWindowIcon(QIcon(":/res/icon.ico"));
    w.show();
    return a.exec();
}

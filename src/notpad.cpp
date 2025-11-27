#include "notpad.hpp"
#include "forms/ui_notpad.h"

NotPad::NotPad(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::NotPad)
{
    ui->setupUi(this);
}

NotPad::~NotPad()
{
    delete ui;
}

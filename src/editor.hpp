#ifndef EDITOR_H
#define EDITOR_H

#include <QPlainTextEdit>
#include <QFile>


class Editor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
    {}

    std::shared_ptr<QFile> m_file;
    bool m_fileEdited;
};

#endif // EDITOR_H

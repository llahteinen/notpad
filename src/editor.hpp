#ifndef EDITOR_HPP
#define EDITOR_HPP

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

#endif // EDITOR_HPP

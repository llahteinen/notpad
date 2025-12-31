#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "file.hpp"
#include <QPlainTextEdit>
#include <QFile>


class Editor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr)
        : QPlainTextEdit(parent)
        , m_file{}
    {}

    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    File::Status save();
    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    File::Status saveAs();

    bool isModified() const;

    std::unique_ptr<QFile> m_file;
};

#endif // EDITOR_HPP

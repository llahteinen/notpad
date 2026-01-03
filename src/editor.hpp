#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "file.hpp"
#include <QPlainTextEdit>
#include <QFile>


class Editor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr);
    Editor(const QString& text, std::unique_ptr<QFile> file_p, QWidget *parent = nullptr);

    static Editor* createEditor(File::Status& o_status, const QString& fileName, QWidget* parent = nullptr);

    /// \return True if can be saved over previous file. False if "save as" / file selection is needed first.
    bool saveOrSaveAs();

    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    File::Status save();
    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    File::Status saveAs(const QString& fileName);

    bool isModified() const;

    void setWordWrap(bool enabled);
    bool isWordWrap() const;
    void updateTabWidth();
    void setFont(const QFont&);     //!< Hide base class setFont

    std::unique_ptr<QFile> m_file;
};

#endif // EDITOR_HPP

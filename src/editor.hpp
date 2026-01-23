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

    void setName(const QString& name);
    QString name() const;
    const QFile* file() const;
    QString encodingName() const;
    bool isModified() const;

    void setWordWrap(bool enabled);
    bool isWordWrap() const;
    void updateTabWidth();
    void setFont(const QFont&);     //!< Hide base class setFont

private:
    QString m_name;
    std::unique_ptr<QFile> m_file;
    QStringConverter::Encoding m_encoding;
    bool m_hasBom;

signals:
    void nameChanged(const QString& new_name);
};

#endif // EDITOR_HPP

#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "file.hpp"
#include "utils/highlighter.hpp"
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

    qsizetype getMatchCount(const QString& sterm, QTextDocument::FindFlags flags);
    const QList<QTextCursor>& getSearchResults(const QString& sterm, QTextDocument::FindFlags flags);

    void setName(const QString& name);
    QString name() const;
    const QFile* file() const;
    QString encodingName() const;
    bool isModified() const;

    void setWordWrap(bool enabled);
    bool isWordWrap() const;
    void updateTabWidth();
    void setFont(const QFont&);     //!< Hide base class setFont

    Highlighter* highLighter;

private slots:
    void onContentsChange(int position, int charsRemoved, int charsAdded);
    void invalidateSearchResults();

private:
    /// \param sterm Search term, can be regexp
    /// \param flags Find options
    /// \return -1 on errors, -2 on overflow, otherwise number of matches in the document
    qsizetype countMatches(const QString& sterm, QTextDocument::FindFlags flags);

    /// NOTE: This function is very slow and memory intensive on large files!
    QList<QTextCursor> findAll(const QString& sterm, QTextDocument::FindFlags flags);

    QString m_name;
    std::unique_ptr<QFile> m_file;
    QStringConverter::Encoding m_encoding;
    bool m_hasBom;

    struct Search
    {
        qsizetype matchCount{-1};   /// qsizetype is signed
        QString term{};
        QTextDocument::FindFlags flags{};
    } m_search;

signals:
    void nameChanged(const QString& new_name);
};

#endif // EDITOR_HPP

#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "file.hpp"
#include "utils/highlighter.hpp"
#include "utils/textstream.hpp"
#include "utils/search.hpp"
#include <QPlainTextEdit>
#include <QFile>
#include <QPointer>


class Editor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit Editor(QWidget *parent = nullptr);
    Editor(TextStream* stream, std::unique_ptr<QFile> file_p, QWidget *parent = nullptr);
    ~Editor();
    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    QString toPlainText() const; /// Hides base class method

    static Editor* createEditor(File::Status& o_status, const QString& fileName, QWidget* parent = nullptr);

    /// \return True if can be saved over previous file. False if "save as" / file selection is needed first.
    bool saveOrSaveAs();

    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    File::Status save();
    /// \return true if file was saved, false if saving was canceled by user or resulted in error
    File::Status saveAs(const QString& fileName);

    void reload();

    /// \brief Count occurrences of a substring. Thread safe.
    qsizetype getMatchCount(const QString& sterm, QTextDocument::FindFlags flags);

    void setName(const QString& name);
    QString name() const;
    QString filePath() const;
    const QFile* file() const;
    QString encodingName() const;
    QString endOfLineName() const;
    bool isModified() const;

    void setWordWrap(bool enabled);
    bool isWordWrap() const;
    void updateTabWidth();

    void setHighlightRegex(const QString& regexStr, QTextDocument::FindFlags flags = {});

    void abortTasks();

public slots:
    void setHighlighterEnabled(bool enabled);
    void setFont(const QFont& font); /// Hides QWidget setFont that is not a slot

private slots:
    void onDataQueued();
    void onContentsChange(int position, int charsRemoved, int charsAdded);

protected:
    virtual void changeEvent(QEvent *event) override;
    virtual void wheelEvent(QWheelEvent *e) override;

private:
    std::chrono::high_resolution_clock::time_point m_start_t{}, m_end_t{}; /// DEBUG

    static std::pair<TextStream*, QThread*> createStreamAndThread(const QString& fileName);

    /// NOTE: This function is very slow and memory intensive on large files!
    QList<QTextCursor> findAll(const QString& sterm, QTextDocument::FindFlags flags);

    QString m_name;                         /// file name not full path
    QPointer<QThread> m_textStreamThread;   /// Thread finished signal connects to deleteLater
    QPointer<TextStream> m_textStream;      /// QPointers become null automatically when the QObject is deleted
    std::unique_ptr<QFile> m_file;
    QStringConverter::Encoding m_encoding;
    EndOfLine m_endOfLine;
    bool m_hasBom;

    QTextCharFormat m_format;

    Search m_search;
    QMutex m_searchMutex{};

    Highlighter* highLighter;   /// This is destroyed by its parent document

    std::atomic_bool m_aborted;
    bool m_loadingInProgress;
    bool m_reloading;
    int m_loadingPos;

signals:
    void nameChanged(const QString& new_name);
    void hasFileChanged(bool has);
    void dataLoadingUpdate(int progress);
    void dataLoadingFinished();
};

#endif // EDITOR_HPP

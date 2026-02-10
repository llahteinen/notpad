#ifndef HIGHLIGHTER_HPP
#define HIGHLIGHTER_HPP

#include <QSyntaxHighlighter>
#include <QRegularExpression>

class Editor;


class Highlighter : public QSyntaxHighlighter
{
public:

    Highlighter() : QSyntaxHighlighter(static_cast<QObject*>(nullptr)) {};
    explicit Highlighter(QTextDocument* parent)
        : QSyntaxHighlighter(parent)
    {
        m_highlightFormat.setBackground(Qt::yellow);
//        m_highlightFormat.setForeground(Qt::black);
    }

    ~Highlighter() = default;

    void highlightBlock(const QString& text) override;

    void rehighlight(); /// reimplement of QSyntaxHighlighter::rehighlight()

    void setSearchTerm(const QString& sterm);

    void setRegex(const QString& regexStr, QTextDocument::FindFlags flags);

    QString m_searchTerm{};
    QString m_regexStr{};
    QRegularExpression m_regex{};
    QTextCharFormat m_highlightFormat{};
    int m_counter{-1};
    std::atomic<bool> m_running{false};
//    std::atomic<bool> m_abort{false};
};

#endif // HIGHLIGHTER_HPP

#ifndef HIGHLIGHTER_HPP
#define HIGHLIGHTER_HPP

#include "qtforks/lsyntaxhighlighter.h"
#include <QRegularExpression>

class Editor;


class Highlighter : public LSyntaxHighlighter
{
public:

    struct HighlightRule
    {
        QRegularExpression regex;
        QTextCharFormat format;
    };

    Highlighter() : LSyntaxHighlighter(static_cast<QObject*>(nullptr)) {};
    explicit Highlighter(QTextDocument* parent);

    ~Highlighter() = default;

    void highlightBlock(const QString& text) override;

    void rehighlight(); /// reimplement of LSyntaxHighlighter::rehighlight()
    void rehighlight(int topPos); /// reimplement of LSyntaxHighlighter::rehighlight(int)

    void setSearchTerm(const QString& sterm);

    void setRegex(const QString& regexStr, QTextDocument::FindFlags flags = {}, int topPos = 0);
    void setSelectionRegex(const QString& regexStr, QTextDocument::FindFlags flags = {}, int topPos = 0);

    QString m_searchTerm{};

    QString m_searchRegexStr{};
    HighlightRule m_searchRegex{{}, {}/*, false*/};

    QString m_selectionRegexStr{};
    HighlightRule m_selectionRegex{{}, {}/*, true*/};

    const QVector<HighlightRule*> m_highlightRules{}; /// Prioritized list. Last one in the list wins

    int m_counter{-1};
    std::atomic<bool> m_running{false};
};

#endif // HIGHLIGHTER_HPP

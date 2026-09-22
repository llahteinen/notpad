#include "highlighter.hpp"
#include "utils/regex.hpp"
#include <QRegularExpression>


Highlighter::Highlighter(QTextDocument* parent)
    : LSyntaxHighlighter(parent)
    , m_highlightRules{&m_selectionRegex, &m_searchRegex}
{
    m_searchRegex.format.setBackground(Qt::yellow);
    m_searchRegex.format.setForeground(Qt::black);

    QTextCharFormat format;
    auto color = QColor::fromHsl(0, 0, 170);
    color.setAlpha(64);
    QBrush brush(color);
    format.setBackground(brush);
    format.clearForeground();
    m_selectionRegex.format = format;
}

void Highlighter::setSearchTerm(const QString& sterm)
{
    if(sterm == m_searchTerm)
    {
        return;
    }

    m_searchTerm = sterm;
//    m_regex = QRegularExpression(QRegularExpression::escape(m_searchTerm));
    m_searchRegex.regex = QRegularExpression(QRegularExpression::escape(m_searchTerm), QRegularExpression::CaseInsensitiveOption);

    rehighlight();
}

void Highlighter::setRegex(const QString& regexStr, QTextDocument::FindFlags flags, int topPos)
{
    if(regexStr == m_searchRegexStr)
    {
        return;
    }

    const auto reg = Regex::stringToRegex(regexStr, flags);
    if(!reg.isValid())
    {
        return;
    }
    m_searchRegex.regex = reg;
    m_searchRegexStr = regexStr;
    m_searchTerm = "";

    rehighlight(topPos);
}

void Highlighter::setSelectionRegex(const QString& regexStr, QTextDocument::FindFlags flags, int topPos)
{
    if(regexStr == m_selectionRegexStr)
    {
        return;
    }

    const auto reg = Regex::stringToRegex(regexStr, flags);
    if(!reg.isValid())
    {
        return;
    }
    m_selectionRegex.regex = reg;
    m_selectionRegexStr = regexStr;

    rehighlight(topPos);
}

void Highlighter::rehighlight()
{
    rehighlight(0);
}

void Highlighter::rehighlight(int topPos)
{
    if(m_running)
    {
        qDebug() << "rehighlight already running";
        return;
    }

    m_running = true;
    m_counter = -1;
    LSyntaxHighlighter::rehighlight(topPos);
    m_running = false;
}

void Highlighter::highlightBlock(const QString& text)
{
    if(Q_UNLIKELY(text.isEmpty())) return;

    for(const auto* rule : m_highlightRules)
    {
        if(Q_UNLIKELY(!rule->regex.isValid())) continue;

        auto matchIterator = rule->regex.globalMatch(text);
        while(matchIterator.hasNext())
        {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule->format);
        }
    }

    ++m_counter;

}


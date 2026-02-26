#include "highlighter.hpp"
#include "utils/regex.hpp"
#include <QRegularExpression>


void Highlighter::setSearchTerm(const QString& sterm)
{
    if(sterm == m_searchTerm)
    {
        return;
    }

    m_searchTerm = sterm;
//    m_regex = QRegularExpression(QRegularExpression::escape(m_searchTerm));
    m_regex = QRegularExpression(QRegularExpression::escape(m_searchTerm), QRegularExpression::CaseInsensitiveOption);

    rehighlight();
}

void Highlighter::setRegex(const QString& regexStr, QTextDocument::FindFlags flags)
{
    if(regexStr == m_regexStr)
    {
        return;
    }

    const auto reg = Regex::stringToRegex(regexStr, flags);
    if(!reg.isValid())
    {
        return;
    }
    m_regex = reg;
    m_regexStr = regexStr;
    m_searchTerm = "";

    rehighlight();
}

void Highlighter::rehighlight()
{
    if(m_running)
    {
//        m_abort = true; /// Even our abort path is a bit slow. Probably need to reimplement parts of QSyntaxHighlighter if want to be better.
        qInfo() << "rehighlight already running";
        return;
    }

    m_running = true;
    m_counter = -1;
    LSyntaxHighlighter::rehighlight();
//    m_abort = false;
    m_running = false;
}

/// Jostain syystä tätä kutsutaan automaattisesti bootissa
void Highlighter::highlightBlock(const QString& text)
{
//    if(m_abort) return;
    if(text.isEmpty()) return;
    if(m_searchTerm.isEmpty() && m_regexStr.isEmpty()) return;
    if(!m_regex.isValid()) return;

    auto matchIterator = m_regex.globalMatch(text);

    while(matchIterator.hasNext())
    {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_highlightFormat);
    }

    ++m_counter;

}


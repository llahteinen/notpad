#include "search.hpp"
#include "regex.hpp"


qsizetype Search::countMatches(QStringView document, const QString& sterm, QTextDocument::FindFlags flags)
{
    qDebug() << "countMatches";

    /// Regex method (this is faster than QString::count(QString))
    const auto reg = Regex::stringToRegex(sterm, flags);
    if(!reg.isValid())
    {
        return -1;
    }

    const auto matchIterator = reg.globalMatchView(document);
    if(!matchIterator.isValid())
    {
        return -1;
    }

    qsizetype count = 0;
    static constexpr auto max_val = std::numeric_limits<decltype(count)>::max();
    for([[maybe_unused]]const auto& _ : matchIterator)
    {
        if(count == max_val)
        {
            count = -2;
            qInfo() << "Too many search results: >" << max_val;
            break;
        }
        ++count;
    }

    qDebug() << "resultCount" << count;
    return count;
}


#include "search.hpp"


qsizetype Search::countMatches(QStringView document, const QString& sterm, const QRegularExpression& regex, QTextDocument::FindFlags flags)
{
    qDebug() << "countMatches";
    Q_UNUSED(sterm);
    Q_UNUSED(flags);

    /// Regex method (this is faster than QString::count(QString))
    if(!regex.isValid())
    {
        return -1;
    }

    const auto matchIterator = regex.globalMatchView(document);
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
            qWarning() << "Too many search results: >" << max_val;
            break;
        }
        ++count;
    }

    qDebug() << "resultCount" << count;
    return count;
}


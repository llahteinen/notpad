#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <QtTypes>
#include <QTextDocument>


struct Search
{
    bool m_countInProgress{false};
    qsizetype matchCount{-1};   /// qsizetype is signed
    QString term{};
    QTextDocument::FindFlags flags{};

    void invalidate()
    {
        term = {};
        flags = {};
    }

    /// \return -1 on errors, -2 on overflow, otherwise number of matches in the document
    static qsizetype countMatches(QStringView document, const QString& sterm, QTextDocument::FindFlags flags);

};

#endif // SEARCH_HPP

#ifndef SEARCH_HPP
#define SEARCH_HPP

#include <QtTypes>
#include <QTextDocument>
#include <QRegularExpression>


struct Search
{
    bool m_countInProgress{false};
    qsizetype matchCount{-1};   /// qsizetype is signed
    int revision{-1};
    QString term{};
    QRegularExpression termRegex{};
    QTextDocument::FindFlags flags{};

    /// \return -1 on errors, -2 on overflow, otherwise number of matches in the document
    static qsizetype countMatches(QStringView document, const QString& sterm, const QRegularExpression& regex, QTextDocument::FindFlags flags);

};

#endif // SEARCH_HPP

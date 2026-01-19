#ifndef NAMEFILTERLIST_HPP
#define NAMEFILTERLIST_HPP

#include <QStringList>


class NameFilterList : public QStringList
{
public:
    NameFilterList() :
        QStringList
        {
            "All files (*)",
            "Text files (*.txt)",
            "Log files (*.log)",
            "Markdown files (*.md)",
            "JSON files (*.json)",
            "C/C++ files (*.cpp *.hpp *.c *.h)",
            "HTML files (*.htm *.html *.php)",
            "CSS files (*.css)",
        }
    {}
    NameFilterList(const QStringList& filterList);
    NameFilterList(std::initializer_list<QString> args);

    const QString& getFilter(QString suffix, const QString& fallback = "") const;

    static QString getSuffix(QString filter, int index = 0, const QString& fallback = "");
    static QStringList getSuffixList(const QString& nameFilter);

};

#endif // NAMEFILTERLIST_HPP

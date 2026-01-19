#include "namefilterlist.hpp"


NameFilterList::NameFilterList(const QStringList& filterList) :
    QStringList(filterList)
{}

NameFilterList::NameFilterList(std::initializer_list<QString> args) :
    QStringList(args)
{}

const QString& NameFilterList::getFilter(QString suffix, const QString& fallback) const
{
    if(suffix.isEmpty())
    {
        return fallback;
    }

    /// Suffix should always be "*.foo" to not match anything in the filter description
    /// NOTE: "*" is the special case
    suffix = suffix.toLower();
    if(suffix.at(0) != '*')
    {
        suffix.prepend('*');
    }
    if(suffix.length() > 1 && suffix.at(1) != '.')
    {
        suffix.insert(1, '.');
    }
    for(const QString& nameFilter : *this)
    {
        const auto suffixList = getSuffixList(nameFilter);
        if(suffixList.contains(suffix, Qt::CaseSensitive))
        {
            return nameFilter;
        }
    }
    return fallback;
}

/// static
QString NameFilterList::getSuffix(QString nameFilter, int index, const QString& fallback)
{
    const auto suffixList = getSuffixList(nameFilter);
    if(suffixList.length() > index)
    {
        auto suffix = suffixList.at(index);
        while(suffix.startsWith('*')) suffix.removeFirst(); /// If suffix is just * returns empty
        while(suffix.startsWith('.')) suffix.removeFirst();
        while(suffix.startsWith('*')) suffix.removeFirst(); /// If suffix is just *.* returns empty
        return suffix;
    }
    return fallback;
}

/// static
QStringList NameFilterList::getSuffixList(const QString& nameFilter)
{
    /// Extract the part between parentheses
    const int start = nameFilter.indexOf('(');
    const int end = nameFilter.indexOf(')');
    if (start == -1 || end == -1) return {};

    const QString extPart = nameFilter.mid(start + 1, end - start - 1);
    return extPart.split(' ', Qt::SkipEmptyParts);
}

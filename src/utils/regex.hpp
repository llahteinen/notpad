#ifndef REGEX_HPP
#define REGEX_HPP

#include <QRegularExpression>
#include <QTextDocument>


namespace Regex
{
    inline QRegularExpression stringToRegex(const QString& sterm, QTextDocument::FindFlags flags)
    {
        QRegularExpression::PatternOptions reg_opt{};

        /// NOTE: Case insensitive is default for QTextDocument::FindFlags
        if(!flags.testFlag(QTextDocument::FindFlag::FindCaseSensitively))
        {
            reg_opt.setFlag(QRegularExpression::PatternOption::CaseInsensitiveOption);
        }
        /// Else find case sensitively

        const QRegularExpression reg{QRegularExpression::escape(sterm), reg_opt}; /// Normal string-like search using regex
    //    const QRegularExpression reg{sterm}; /// Full regex pattern search. Should check PatternOptions at some point
    //    reg.optimize(); /// Not sure what this does, does not seem to affect performance
        if(!reg.isValid())
        {
            qInfo() << "Invalid regexp:" << reg.errorString();
        }
        return reg;
    }
}

#endif // REGEX_HPP

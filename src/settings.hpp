#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QFont>
#include <QList>
#include <QString>
#include <QStringList>
#include <QDir>


class Settings
{
public:
    static Settings& get()
    {
        static Settings instance;
        return instance;
    }

    /// Constants
    static constexpr int fontSizeDefault{10};
    const QList<int> standardFontSizes{3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, 36, 48, 72};
    const QStringList nameFilters
    {
        "All files (*)",
        "Text files (*.txt)",
        "Log files (*.log)",
        "Markdown files (*.md)",
        "JSON files (*.json)",
        "C/C++ files (*.cpp *.hpp *.c *.h)",
        "HTML files (*.htm *.html *.php)",
        "CSS files (*.css)",
    };
    const QString defaultNameFilter{nameFilters.at(1)};
//    const QStringList mimeTypeFilters{ /// This is alternative to nameFilters, both can't be used together
//        "text/plain", /// Returns a huge amount of suffixes
//        "text/csv",
//        "text/html",
//        "application/json",
//        "application/octet-stream" /// will show "All files (*)"
//    };


    /// Editables
    QFont font{"Consolas", fontSizeDefault};
//    int selectedFontSize{};
    int tabWidthChars{4};  /// Measured in characters or multiples of avg character width
    bool confirmAppClose{false};
    QString defaultDocName{"Untitled"};


    /// Runtime (not to be persisted)
    QString currentNameFilter{defaultNameFilter};
    QDir currentDir{};

};

inline auto& SETTINGS = Settings::get();


#endif // SETTINGS_HPP

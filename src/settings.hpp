#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "utils/namefilterlist.hpp"
#include <QFont>
#include <QList>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QStandardPaths>

class QSettings;


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
    const QList<int> standardFontSizes{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, 32, 36, 42, 48, 56, 64, 72, 84, 100, 116, 132, 164, 196};
    const NameFilterList nameFilters{};
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
    int tabWidthChars{4};  /// Measured in characters or multiples of avg character width
    bool confirmAppClose{false};
    QString defaultDocName{"Untitled"};
    int lineNumberOffset{1};    /// Whether first line is line 0 or line 1


    /// Runtime (not to be persisted)
    QString currentNameFilter{defaultNameFilter};
    QDir currentDir{QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)};


    struct Persistables
    {
        /// Background
        unsigned int startupCounter{0};
        QByteArray windowGeometry{};
        QStringList sessionTabs{};

        /// User editables
        /// Options (menu bar choices)
        bool wordWrap{true};    /// This would probably be best if it was saved per tab
        int zoomFontSize{fontSizeDefault}; /// The font point size after user has been fiddling with the "zoom" controls


        /// \brief Loads from persistent storage
        void fromQSettings(const QSettings& settings);
        /// \brief Saves to persistent storage
        void toQSettings(QSettings& settings);

    } pers{};

};

inline auto& SETTINGS = Settings::get();


#endif // SETTINGS_HPP

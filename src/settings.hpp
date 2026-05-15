#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "utils/namefilterlist.hpp"
#include <QFont>
#include <QFontDatabase>
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
    const QString operatingSystem;
    const QFont systemFont;
    const int fontSizeDefault;
    const QList<int> standardFontSizes{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 28, 32, 36, 42, 48, 56, 64, 72, 84, 100, 116, 132, 164, 196};
    const NameFilterList nameFilters{};
    const QString defaultNameFilter{nameFilters.at(1)};
//    const QStringList mimeTypeFilters{ /// This is alternative to nameFilters, both can't be used together
//        "text/plain", /// Returns a huge amount of suffixes
//        "text/csv",
//        "text/html",
//        "application/json",
//        "application/octet-stream" /// will show "All files (*)"
//    };
    QStringList monospaceFamilies;


    /// Editables
    QFont font;            /// This font will be used when constructing new editors
    int tabWidthChars{4};  /// Measured in characters or multiples of avg character width
    bool confirmAppClose{false};
    QString defaultDocName{"Untitled"};
    int lineNumberOffset{1};    /// Whether first line is line 0 or line 1


    /// Runtime (not to be persisted)
    QString currentNameFilter{defaultNameFilter};
    QDir currentDir{QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)};


    /// Give sensible defaults here for fresh installations
    struct Persistables
    {
        /// Background
        unsigned int startupCounter{0};
        QByteArray windowGeometry{};
        QStringList sessionTabs{};

        /// User editables
        /// Options (menu bar choices)
        bool wordWrap{true};    /// This would probably be best if it was saved per tab
        int zoomFontSize{11};   /// The font point size after user has been fiddling with the "zoom" controls
        QFont::StyleHint fontStyle{QFont::StyleHint::Monospace};


        /// \brief Loads from persistent storage
        void fromQSettings(const QSettings& settings);
        /// \brief Saves to persistent storage
        void toQSettings(QSettings& settings);

    } pers{};

    static constexpr auto getMonospaceFamilies = [](const QString& os) -> QStringList {
        if(os == "macos")
        {
            return {"Menlo", "PT Mono", "Andale Mono", "Courier New"};
        }
        else if(os == "linux")
        {
            return {"Noto Mono", "DejaVu Sans Mono", "Monospace"};
        }
        else
        {
            return {"Cascadia Mono", "Consolas", "Lucida Console", "Courier new"};
        }
    };

    Settings()
        : operatingSystem{QSysInfo::productType()}
        , systemFont{QFontDatabase::systemFont(QFontDatabase::GeneralFont)}
        , fontSizeDefault{systemFont.pointSize()}
        , monospaceFamilies{getMonospaceFamilies(operatingSystem)}
    {
        font.setPointSize(fontSizeDefault);
        font.setStyleHint(pers.fontStyle);
        font.setFamilies(monospaceFamilies);

        pers.zoomFontSize = fontSizeDefault;
    }
};

//inline auto& SETTINGS = Settings::get(); /// Causes Settings::get() to be run too early (static)
#define SETTINGS Settings::get()


#endif // SETTINGS_HPP

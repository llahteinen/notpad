#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "utils/namefilterlist.hpp"
#include <QObject>
#include <QFont>
#include <QFontDatabase>
#include <QList>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QStandardPaths>

class QSettings;


class Settings : public QObject
{
    Q_OBJECT
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


    /// Editables
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
        QFont font;             /// This font will be used when constructing new editors


        /// \brief Loads from persistent storage
        void fromQSettings(const QSettings& settings);
        /// \brief Saves to persistent storage
        void toQSettings(QSettings& settings);

    } pers{};

    const QStringList& getMonospaceFamilies() {
        static QStringList fontList;
        if(fontList.empty())
        {
            if(operatingSystem == "macos")
            {
                fontList = {"Menlo", "PT Mono", "Andale Mono", "Courier New"};
            }
            else if(operatingSystem == "linux")
            {
                fontList = {"Noto Mono", "DejaVu Sans Mono", "Monospace"};
            }
            else
            {
                fontList = {"Consolas", "Cascadia Mono", "Lucida Console", "Courier new"};
            }
        }
        return fontList;
    };

    Settings()
        : operatingSystem{QSysInfo::productType()}
        , systemFont{QFontDatabase::systemFont(QFontDatabase::GeneralFont)}
        , fontSizeDefault{systemFont.pointSize()}
    {
        pers.font.setPointSize(fontSizeDefault);
        pers.font.setStyleHint(QFont::StyleHint::Monospace);
        pers.font.setFamilies(getMonospaceFamilies());
    }

    void setFontStyle(QFont::StyleHint style);

signals:
    void fontChanged(const QFont& font);
};

//inline auto& SETTINGS = Settings::get(); /// Causes Settings::get() to be run too early (static)
#define SETTINGS Settings::get()


#endif // SETTINGS_HPP

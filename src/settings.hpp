#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "types.hpp"
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
    const qreal fontSizeDefault;
    const QList<qreal> standardFontSizes{2, 3, 4, 4.5, 5, 6, 7, 7.5, 8, 9, 10, 10.5, 11, 12, 13, 13.5, 14, 15, 16, 17, 18, 19, 20, 22, 24, 28, 32, 36, 42, 48, 56, 64, 72, 84, 100, 116, 132, 164, 196};
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
    EndOfLine endOfLine{EndOfLine::UNIX};


    /// Give sensible defaults here for fresh installations
    /// and if migrator for some reason does not find old values
    struct Persistables
    {
        /// Version of the Persistables structure
        /// Increase this if doing any breaking changes to Persistables structure
        static constexpr unsigned int SETTINGS_VERSION = 1;
        const QString programVersion{PROJECT_FULL_VERSION};

        /// Background
        unsigned int startupCounter{0};
        QByteArray windowGeometry{};
        QStringList sessionTabs{};

        /// User editables
        /// Options (menu bar choices)
        bool wordWrap{true};    /// This would probably be best if it was saved per tab
        QFont font{};           /// This font will be used when constructing new editors
        Qt::ColorScheme colorScheme{Qt::ColorScheme::Unknown}; /// Unknown == system default


    private:
        /// \brief Loads from persistent storage and runs migrators if needed
        void fromQSettings(QSettings& settings);
        /// \brief Saves to persistent storage
        void toQSettings(QSettings& settings) const;

        /// \brief Checks if the persisted settings had a lower version than the current
        bool isMigrationNeeded(unsigned int storedSettingsVersion) const;

        using MigrationFunction = std::function<void(QSettings&)>;
        /// Vector of migration functions, indexed by target version
        /// migrator[0] migrates from v0 → v1, migrator[1] migrates from v1 → v2, etc
        /// Reads from QSettings persistent storage and immediately writes new settings back
        const QVector<MigrationFunction> migrators;

        /// \brief Loads from persistent storage assuming it is in current Settings version format
        void readCurrentQSettings(QSettings& settings);

        Persistables();
        friend class Settings;
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
        , fontSizeDefault{systemFont.pointSizeF() + 1}
    {
        pers.font.setPointSizeF(fontSizeDefault);
        pers.font.setStyleHint(QFont::StyleHint::Monospace);
        pers.font.setFamilies(getMonospaceFamilies());

        if(operatingSystem == "windows")
        {
            endOfLine = EndOfLine::WINDOWS;
        }
    }

    /// \brief Loads from persistent storage and runs migrators if needed
    void load();
    /// \brief Saves to persistent storage
    void save();

    void setWordWrap(bool wordWrap);
    void incrementFontSize(int increment);
    void restoreFontSize();
    void setFontStyle(QFont::StyleHint style);
    void setColorScheme(Qt::ColorScheme scheme);

signals:
    void wordWrapChanged(bool wordWrap);
    void fontChanged(const QFont& font);
    void colorSchemeChanged(Qt::ColorScheme scheme);
};

//inline auto& SETTINGS = Settings::get(); /// Causes Settings::get() to be run too early (static)
#define SETTINGS Settings::get()


#endif // SETTINGS_HPP

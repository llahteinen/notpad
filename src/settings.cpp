#include "settings.hpp"
#include "utils/utils.hpp"
#include <QSettings>


Settings::Persistables::Persistables()
    : migrators{
      /// v0 to v1
      [this](QSettings& settings) {
          qDebug() << "Migrator 0 to 1";
          /// Set startupCounter to the old settings value, or if old setting not found, to the default startupCounter value
          settings.setValue("startupCounter",   settings.value("main/startupCounter", startupCounter).toUInt());
          /// Delete old startupCounter key
          settings.remove("main/startupCounter");
      },
    }
{

}

bool Settings::Persistables::isMigrationNeeded(unsigned int storedSettingsVersion) const
{
    /// Migrate if settings are from an older version of the program where settings version was lower
    /// Not really sure what to do if user has downgraded versions. Probably just use default values then and don't support migration
    return storedSettingsVersion < SETTINGS_VERSION;
}

void Settings::Persistables::fromQSettings(QSettings& settings)
{
    unsigned int settings_ver = settings.value("settingsVersion", 0).toUInt();
    const QString program_ver = settings.value("programVersion", "0.0.0").toString();
    qDebug() << "Persisted settings ver" << settings_ver << program_ver;
    if(settings_ver > SETTINGS_VERSION)
    {
        qWarning() << "Persisted settings version is higher than current:" << settings_ver << "vs" << SETTINGS_VERSION;
    }

    /// Is stored settings version smaller than current settings version
    if(isMigrationNeeded(settings_ver))
    {
        qInfo() << "Settings migration is needed from" << settings_ver << "to" << SETTINGS_VERSION;
        /// Migrated settings are persisted immediately here, but with the old values
        /// New values, such as programVersion, are stored as normal at application exit
        while(isMigrationNeeded(settings_ver))
        {
            /// Let all available migrators run first before asserting or breaking
            Q_ASSERT_X(settings_ver < migrators.size(), " fromQSettings", "No migrator found for settings version");
            if(!(settings_ver < migrators.size()))
            {
                qWarning() << "No migrator available for" << settings_ver << "to" << settings_ver + 1;
                break;
            }

            migrators.at(settings_ver)(settings);
            settings_ver++;
        }
        settings.setValue("settingsVersion",    settings_ver);
        settings.setValue("programVersion",     programVersion);
    }

    readCurrentQSettings(settings);
}

void Settings::Persistables::readCurrentQSettings(QSettings& settings)
{
    startupCounter  = settings.value("startupCounter", startupCounter).toUInt();
    windowGeometry  = settings.value("main/windowGeometry").toByteArray();
    sessionTabs     = settings.value("main/sessionTabs").toStringList();
    wordWrap        = settings.value("options/wordWrap", wordWrap).toBool();
    font.fromString(  settings.value("options/font", font.toString()).toString()); /// Uses font.toString as the default fallback value
    colorScheme = static_cast<Qt::ColorScheme>( settings.value("options/colorScheme", static_cast<int>(colorScheme)).toInt() );
}

void Settings::Persistables::toQSettings(QSettings& settings) const
{
    settings.setValue("settingsVersion",        SETTINGS_VERSION);
    settings.setValue("programVersion",         programVersion);

    settings.setValue("startupCounter",         startupCounter);
    settings.setValue("main/windowGeometry",    windowGeometry);
    settings.setValue("main/sessionTabs",       sessionTabs);
    settings.setValue("options/wordWrap",       wordWrap);
    settings.setValue("options/font",           font.toString());
    settings.setValue("options/colorScheme",    static_cast<int>(colorScheme));
}

void Settings::incrementFontSize(int increment)
{
    auto size = pers.font.pointSizeF();
    size = Utils::roundToHalf(size);
    auto index = standardFontSizes.indexOf(size);
    Q_ASSERT_X(index >= 0, "incrementFontSize", "Got weird font size");
    if(index < 0)
    {
        qWarning() << "Got weird font size" << size;
        index = standardFontSizes.indexOf(fontSizeDefault);
    }
    index += increment;
    index = qMax(index, 0);
    index = qMin(index, standardFontSizes.length()-1);
    qDebug() << "index" << index;
    size = standardFontSizes.at(index);
    qDebug() << "size" << size;
    pers.font.setPointSizeF(size);

    emit fontChanged(pers.font);
}

void Settings::restoreFontSize()
{
    pers.font.setPointSizeF(fontSizeDefault);
    qDebug() << "pointSize" << pers.font.pointSizeF();

    emit fontChanged(pers.font);
}

void Settings::setFontStyle(QFont::StyleHint style)
{
    pers.font.setStyleHint(style);
    if(style == QFont::StyleHint::Monospace)
    {
        pers.font.setFamilies(getMonospaceFamilies());
    }
    else
    {
        const auto default_family = pers.font.defaultFamily();
        qDebug() << "defaultFamily" << pers.font.defaultFamily();
        pers.font.setFamily(default_family); /// Family takes precedence over any other attributes, so set it to the default value for the style hint
    }

    emit fontChanged(pers.font);
}

void Settings::setColorScheme(Qt::ColorScheme scheme)
{
    pers.colorScheme = scheme;
}


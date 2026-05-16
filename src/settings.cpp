#include "settings.hpp"
#include <QSettings>


void Settings::Persistables::fromQSettings(const QSettings& settings)
{
    startupCounter  = settings.value("main/startupCounter", startupCounter).toUInt();
    windowGeometry  = settings.value("main/windowGeometry").toByteArray();
    sessionTabs     = settings.value("main/sessionTabs").toStringList();
    wordWrap        = settings.value("options/wordWrap", wordWrap).toBool();
    font.fromString(  settings.value("options/font", font.toString()).toString()); /// Uses font.toString as the default fallback value
}

void Settings::Persistables::toQSettings(QSettings& settings)
{
    settings.setValue("main/startupCounter",    startupCounter);
    settings.setValue("main/windowGeometry",    windowGeometry);
    settings.setValue("main/sessionTabs",       sessionTabs);
    settings.setValue("options/wordWrap",       wordWrap);
    settings.setValue("options/font",           font.toString());
}

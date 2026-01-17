#include "settings.hpp"
#include <QSettings>


void Settings::Persistables::fromQSettings(const QSettings& settings)
{
    startupCounter  = settings.value("main/startupCounter", 0u).toUInt();
    windowGeometry  = settings.value("main/windowGeometry").toByteArray();
    sessionTabs     = settings.value("main/sessionTabs").toStringList();
}

void Settings::Persistables::toQSettings(QSettings& settings)
{
    settings.setValue("main/startupCounter",    startupCounter);
    settings.setValue("main/windowGeometry",    windowGeometry);
    settings.setValue("main/sessionTabs",       sessionTabs);
}

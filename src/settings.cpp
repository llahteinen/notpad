#include "settings.hpp"
#include "utils/utils.hpp"
#include <QSettings>


void Settings::Persistables::fromQSettings(const QSettings& settings)
{
    startupCounter  = settings.value("main/startupCounter", startupCounter).toUInt();
    windowGeometry  = settings.value("main/windowGeometry").toByteArray();
    sessionTabs     = settings.value("main/sessionTabs").toStringList();
    wordWrap        = settings.value("options/wordWrap", wordWrap).toBool();
    font.fromString(  settings.value("options/font", font.toString()).toString()); /// Uses font.toString as the default fallback value
    colorScheme = static_cast<Qt::ColorScheme>( settings.value("options/colorScheme", static_cast<int>(colorScheme)).toInt() );
}

void Settings::Persistables::toQSettings(QSettings& settings)
{
    settings.setValue("main/startupCounter",    startupCounter);
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


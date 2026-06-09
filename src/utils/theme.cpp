#include "theme.hpp"
#include <QString>
#include <QIcon>
#include <QDebug>


namespace Theme
{

QString findCounterpartTheme(const QString& themeName)
{
    if(themeName.isEmpty())
    {
        return themeName;
    }

    /// Assume that themename-dark themes have a themename or a themename-light counterpart
    if(themeName.endsWith("-dark", Qt::CaseInsensitive))
    {
        QString other_name = themeName;
        other_name.remove("-dark", Qt::CaseInsensitive);
        qDebug() << "other_name" << other_name;
        if(themeExists(other_name))
        {
            return other_name;
        }
        other_name += "-light";
        if(themeExists(other_name))
        {
            return other_name;
        }
        return ""; /// This should be unlikely because -dark themes should have a counterpart
    }

    /// Assume that themename-light themes have a themename-dark counterpart
    if(themeName.endsWith("-light", Qt::CaseInsensitive))
    {
        QString base_name = themeName;
        base_name.remove("-light", Qt::CaseInsensitive);
        qDebug() << "base_name" << base_name;
        QString other_name = base_name +  "-dark";
        if(themeExists(other_name))
        {
            return other_name;
        }
        return "";
    }

    /// Does not end with -light or -dark
    /// assume it is meant for either light theme or both
    QString other_name = themeName;
    other_name += "-dark";
    if(themeExists(other_name))
    {
        return other_name; /// dark counterpart found
    }
    return themeName; /// Dark counterpart not found, so use the same for both
}

bool themeExists(const QString& themeName)
{
    const auto orig_theme = QIcon::themeName();
    const auto orig_fb_theme = QIcon::fallbackThemeName();
    QIcon::setThemeName(themeName);
    QIcon::setFallbackThemeName("this-must-be-set-to-something-non-existent"); /// Otherwise Qt implicitly uses system's fallback
    /// Windows though seems to always find the system icon

    QIcon testIcon = QIcon::fromTheme("document-new"); /// Try a common icon
    const bool theme_exists = !testIcon.isNull();

    QIcon::setFallbackThemeName(orig_fb_theme);
    QIcon::setThemeName(orig_theme);
    return theme_exists;
}

} // namespace Theme

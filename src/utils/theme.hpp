#ifndef THEME_HPP
#define THEME_HPP

class QString;


namespace Theme
{

    bool themeExists(const QString& themeName);

    /// \brief findCounterpartTheme Find dark theme for light theme or vice versa
    /// \param themeName
    /// \return Theme counterpart name
    QString findCounterpartTheme(const QString& themeName);

};

#endif // THEME_HPP

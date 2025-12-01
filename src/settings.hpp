#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <QFont>
#include <QList>


struct Settings
{
    /// Constants
    static constexpr int fontSizeDefault{10};
    const QList<int> standardFontSizes{3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, 36, 48, 72};

    /// Editables
    QFont font{"Consolas", fontSizeDefault};
//    int selectedFontSize{};
    int tabWidthChars{4};  /// Measured in characters or multiples of avg character width

};


#endif // SETTINGS_HPP

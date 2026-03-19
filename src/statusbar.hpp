#ifndef STATUSBAR_HPP
#define STATUSBAR_HPP

#include <QStatusBar>
#include <QTextCursor>

class QLabel;


class StatusBar : public QStatusBar
{
public:
    struct Data
    {
        struct Encoding
        {
            QString encoding;
            QString endOfLine;
        };
        struct Stats
        {
            int lines;
            int chars;
        };
        struct Cursor
        {
            int ln;
            int col;
            int pos;
            Cursor(int ln, int col, int pos);
            Cursor(const QTextCursor& cur);
        };
        std::optional<Encoding> encoding{std::nullopt};
        std::optional<Stats> stats{std::nullopt};
        std::optional<Cursor> cursor{std::nullopt};
    };

    StatusBar(const QLocale& locale, QWidget* parent = nullptr);

    StatusBar(const StatusBar&) = delete;
    StatusBar& operator=(const StatusBar&) = delete;

    void update(const Data& d);

private:
    const QLocale& m_locale;

    QLabel* m_encodingLabel{};  //!< The rightmost text box in status bar
    QLabel* m_endOfLineLabel{};
    QLabel* m_docStatsLabel{};
    QLabel* m_cursorPositionLabel{};
};

using StatusBarData = StatusBar::Data;

#endif // STATUSBAR_HPP

#include "statusbar.hpp"
#include "settings.hpp"
#include "utils/utils.hpp"
#include <QLocale>
#include <QLabel>


StatusBar::Data::Cursor::Cursor(int ln_, int col_, int pos_) : ln{ln_}, col{col_}, pos{pos_} {}
StatusBar::Data::Cursor::Cursor(const QTextCursor& cur) : Cursor(cur.blockNumber(), cur.positionInBlock(), cur.position()) {}

StatusBar::StatusBar(const QLocale& locale, QWidget* parent) : QStatusBar(parent), m_locale{locale}
{
    setObjectName("statusbar");
    setSizeGripEnabled(true);

    m_encodingLabel = new QLabel(this); /// Rightmost
    m_endOfLineLabel = new QLabel(this);
    m_docStatsLabel = new QLabel(this);
    m_cursorPositionLabel = new QLabel(this);

    const auto metrics = QFontMetrics(this->font());
    qreal width;
    width = metrics.horizontalAdvance(QString("%1 %2")
                                          .arg(QStringConverter::nameForEncoding(QStringConverter::Encoding::Utf32BE), "BOM"));
    m_encodingLabel->setMaximumWidth(width + 2);
    width = metrics.horizontalAdvance(Utils::nameForEndOfLine(EndOfLine::WINDOWS));
    m_endOfLineLabel->setMaximumWidth(width + 2);
    width = metrics.horizontalAdvance(QString("Lines %1   Chars %2")
                                          .arg(m_locale.toString(999'999),
                                               m_locale.toString(999'999'999)));
    m_docStatsLabel->setMaximumWidth(width + 2);
    width = metrics.horizontalAdvance(QString("Ln %1  Col %2  Pos %3")
                                          .arg(m_locale.toString(999'999),
                                               m_locale.toString(9999),
                                               m_locale.toString(999'999'999)));
    m_cursorPositionLabel->setMaximumWidth(width + 2);

    QWidget* spacer = new QLabel(this);
    spacer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    spacer->setMaximumWidth(1);

    addPermanentWidget(spacer, 1);
    addPermanentWidget(m_cursorPositionLabel, 1);
    addPermanentWidget(m_docStatsLabel, 1);
    addPermanentWidget(m_endOfLineLabel, 1);
    addPermanentWidget(m_encodingLabel, 1); /// Rightmost
}

void StatusBar::update(const Data& d)
{
    if(d.encoding.has_value())
    {
        m_encodingLabel->setText(d.encoding->encoding);
        m_endOfLineLabel->setText(d.encoding->endOfLine);
    }
    if(d.stats.has_value())
    {
        m_docStatsLabel->setText(QString("Lines %1   Chars %2")
                                           .arg(m_locale.toString(d.stats->lines),  /// 999999
                                                m_locale.toString(d.stats->chars)));/// 999999999
    }
    if(d.cursor.has_value())
    {
        m_cursorPositionLabel->setText(QString("Ln %1  Col %2  Pos %3")
                                           .arg(m_locale.toString(d.cursor->ln + SETTINGS.lineNumberOffset),   /// 999999
                                                m_locale.toString(d.cursor->col + SETTINGS.lineNumberOffset),  /// 9999
                                                m_locale.toString(d.cursor->pos + SETTINGS.lineNumberOffset)));/// 999999999
    }
}

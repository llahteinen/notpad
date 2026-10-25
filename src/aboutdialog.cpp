#include "aboutdialog.hpp"

#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QFont>
#include <QIcon>
#include <QClipboard>
#include <QLibraryInfo>
#include <QTimer>
#include <QPainter>

namespace
{

QLabel* makeSelectableLabel(const QString &text)
{
    auto* label = new QLabel(text);
    /// Selectable text so the user can select + copy it with the mouse
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    label->setCursor(Qt::IBeamCursor);
    return label;
}

QIcon makeCheckIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen(QGuiApplication::palette().text().color(), 6);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.drawPolyline(QPolygonF() << QPointF(16, 34) << QPointF(28, 46)
                                     << QPointF(48, 18));

    return QIcon(pixmap);
}

/// Draw similar icon like QIcon::fromTheme("edit-copy")
QIcon makeCopyIcon()
{
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal s = 64 / 16.0;  /// the design grid is 16x16
    const QColor color = QGuiApplication::palette().text().color();
    QPen pen(color, s);

    /// Back sheet outline.
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(2.25 * s, 4.5 * s, 7.5 * s, 10 * s), 2 * s, 2 * s);
    /// Erase front sheet area with transparent fill
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::black);
    painter.setCompositionMode(QPainter::CompositionMode_Clear);
    painter.drawRoundedRect(QRectF(6.25 * s, 1.5 * s, 7.5 * s, 10 * s), 2 * s, 2 * s);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    /// Front sheet outline
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(6.25 * s, 1.5 * s, 7.5 * s, 10 * s), 2 * s, 2 * s);

    return QIcon(pixmap);
}

QWidget* makeCopyableLabel(const QString& plainText, const QString& richText = {})
{
    auto* label = new QLabel(richText.isEmpty() ? plainText : richText);
    if(!richText.isEmpty())
    {
        /// Clickable (opens the default browser) AND selectable for copying.
        label->setOpenExternalLinks(true);
    }
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    label->setCursor(Qt::IBeamCursor);

    auto* copyButton = new QToolButton;
    copyButton->setText(AboutDialog::tr("Copy"));
    copyButton->setIconSize({16, 16});
    auto copyIcon = makeCopyIcon();
    copyButton->setIcon(makeCopyIcon());
    copyButton->setToolTip(AboutDialog::tr("Copy to clipboard"));
    copyButton->setAutoRaise(true);

    QObject::connect(copyButton, &QToolButton::clicked, [plainText, copyButton, copyIcon] {
        QGuiApplication::clipboard()->setText(plainText);
        /// Brief feedback, then restore the label after 1.5 s.
        /// Passing copyButton as the timer context means the lambda will
        /// not run if the dialog (and button) is destroyed in the meantime.
        copyButton->setText(AboutDialog::tr("Copied"));
        copyButton->setIcon(makeCheckIcon());
        QTimer::singleShot(1000, copyButton, [copyButton, copyIcon] {
            copyButton->setText(AboutDialog::tr("Copy"));
            copyButton->setIcon(copyIcon);
        });
    });

    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(label);
//    layout->addStretch();
    layout->addWidget(copyButton);
    return row;
}

} // namespace


AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About %1").arg(QStringLiteral(PROJECT_NAME)));

    const QColor fg_color = QGuiApplication::palette().text().color();
    auto style = QString(R"(
QToolButton {
    height: 22px;
    width: 22px;
    border: 0px;
    border-radius: 4px;
}
QToolButton:hover {
    background-color: rgba(%1, %2, %3, 12);
}
QToolButton:pressed {
    background-color: rgba(%1, %2, %3, 6);
}
)").arg(fg_color.red()).arg(fg_color.green()).arg(fg_color.blue());
    setStyleSheet(style);

    /// Header: Large app icon, name + version
    auto icon = QGuiApplication::windowIcon();
    if(icon.isNull() && parent)
    {
        icon = parent->windowIcon(); /// fallback to icon set on MainWindow
    }
    auto* iconLabel = new QLabel;
    iconLabel->setPixmap(icon.pixmap(64));

    QFont titleFont = font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.5);
    titleFont.setBold(true);

    auto* titleLabel = makeSelectableLabel(
        tr("%1 %2").arg(QStringLiteral(PROJECT_NAME),
                        QStringLiteral(PROJECT_SHORT_VERSION)));
    titleLabel->setFont(titleFont);

    auto* idLayout = new QVBoxLayout;
    idLayout->addWidget(titleLabel);

    auto* headerLayout = new QHBoxLayout;
    headerLayout->addWidget(iconLabel, 0, Qt::AlignTop);
    headerLayout->addSpacing(16);
    headerLayout->addLayout(idLayout);
    headerLayout->addStretch();

    /// Version info section
    auto* versionForm = new QFormLayout;
    versionForm->addRow(tr("Build:"),
                        makeCopyableLabel(QStringLiteral(PROJECT_FULL_VERSION)));
    versionForm->addRow(tr("Built against Qt:"),
                        makeCopyableLabel(QLatin1String(QT_VERSION_STR)));
    versionForm->addRow(tr("Running with Qt:"),
                        makeCopyableLabel(tr("%1 (%2 build)")
                                              .arg(QLatin1String(qVersion()))
                                              .arg(QLibraryInfo::isSharedBuild() ? "shared" : "static")));

    /// Link + copyright
    auto* copyrightForm = new QFormLayout;
    auto* url_label = makeCopyableLabel(QLatin1String(PROJECT_URL), QStringLiteral("<a href=\"%1\">%1</a>").arg(QLatin1String(PROJECT_URL)));
    url_label->setToolTip(AboutDialog::tr("Open link in default app"));
    copyrightForm->addRow(tr("Project page:"),
                          url_label);
    copyrightForm->addRow(tr("Copyright ©"),
                          makeCopyableLabel(QString("%1 @ GitHub").arg(QStringLiteral(ORGANIZATION_NAME))));

    /// Buttons
    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    versionForm->setHorizontalSpacing(16);
    versionForm->setVerticalSpacing(0);
    copyrightForm->setHorizontalSpacing(16);
    copyrightForm->setVerticalSpacing(0);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 8, 16, 16);
    layout->setSpacing(0);
    layout->addLayout(headerLayout);
    layout->addSpacing(12);
    layout->addLayout(versionForm);
    layout->addSpacing(12);
    layout->addLayout(copyrightForm);
    layout->addSpacing(16);
    layout->addWidget(buttonBox);
    layout->setSizeConstraint(QLayout::SetFixedSize);
}

#include "mainwindow.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <SingleApplication>

#ifdef Q_OS_WINDOWS
#include <Windows.h>
#endif


void raiseWidget(MainWindow* widget);

int main(int argc, char *argv[])
{
    /// Single instance for this application
    SingleApplication a(argc, argv, true, /// Allow secondary instances
                        (SingleApplication::Mode::User |
                         SingleApplication::Mode::SecondaryNotification)
                        );

    QObject::connect(&a, &SingleApplication::instanceStarted,
                     [](){ qInfo() << "New instance"; } );


    QCommandLineParser parser;
    /// Set this to allow e.g. -multiInst in addition to --multiInst
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    parser.setApplicationDescription(PROJECT_NAME);
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption multi_option("multi",
                                    QCoreApplication::translate("main", "Allow launching multiple instances."));
    parser.addOption(multi_option);
    /// Alias of multi for Notepad++ compatibility (in NP++ only works with single dash -multiInst)
    QCommandLineOption multiInst_option("multiInst",
                                        QCoreApplication::translate("main", "Allow launching multiple instances."));
    parser.addOption(multiInst_option);

    QCommandLineOption nosession_option("nosession",
                                        QCoreApplication::translate("main", "Start without tabs from previous session."));
    parser.addOption(nosession_option);

    /// This is only for printing help. The actual style command is handled automatically by Qt, and it is not obtainable with qApp->arguments()
    QCommandLineOption style_option("style",
                                    QCoreApplication::translate("main", "Set the style of the application."),
                                    QCoreApplication::translate("main", "stylename"));
    parser.addOption(style_option);

    QCommandLineOption view_styles_option("styles",
                                          QCoreApplication::translate("main", "View all available styles that can be selected using --style stylename parameter."));
    parser.addOption(view_styles_option);

    QCommandLineOption theme_option("theme",
                                    QCoreApplication::translate("main", "Set the icon theme of the application (Linux only)."),
                                    QCoreApplication::translate("main", "themename"));
    parser.addOption(theme_option);

    QCommandLineOption view_themes_option("themepaths",
                                          QCoreApplication::translate("main", "View paths that are searched for icon themes."));
    parser.addOption(view_themes_option);

    parser.addPositionalArgument("filename(s)", QCoreApplication::translate("main", "File name(s) to open."));

    parser.process(a);

    if(parser.isSet("styles"))
    {
        QString text = "Available styles:\n";
        for(const auto& style : QStyleFactory::keys())
        {
            text.append(" ").append(style).append('\n');
        }
        text.append("Usage example:\n");
        text.append(PROJECT_NAME).append(" --style Fusion\n");
        parser.showMessageAndExit(QCommandLineParser::MessageType::Information, text);
    }

    if(parser.isSet("themepaths"))
    {
        QString text = "Icon theme search paths:\n";
        for(const auto& path : QIcon::themeSearchPaths())
        {
            text.append(" ").append(path).append('\n');
        }
        parser.showMessageAndExit(QCommandLineParser::MessageType::Information, text);
    }

    if(parser.isSet("theme"))
    {
        const QString theme = parser.value(theme_option);
        QIcon::setThemeName(theme);
        qInfo() << "Requesting theme" << theme;
    }

    const bool multi_instance = parser.isSet("multi") || parser.isSet("multiInst");
    /// If we are in single instance mode and this is secondary instance, send file list to primary
    if(!multi_instance && a.isSecondary())
    {
#ifdef Q_OS_WINDOWS
        /// Enable the primary instance to set itself as foreground window
        AllowSetForegroundWindow( DWORD( a.primaryPid() ) );
#endif

        /// Send argument file list to primary instance
        const QStringList file_list = parser.positionalArguments();
        QByteArray file_barr;
        QDataStream stream(&file_barr, QIODevice::WriteOnly);
        stream << file_list;

        /// This secondary instance sends a message to primary instance
        a.sendMessage(file_barr);

        return 0;
    }

    MainWindow w{parser, parser.isSet("nosession")};

    /// Receive file list from secondary instances
    QObject::connect(&a, &SingleApplication::receivedMessage, &w, [&w](quint32, QByteArray message) {
        w.receiveMessage(message);
    }, Qt::QueuedConnection);

    QObject::connect(&a, &SingleApplication::receivedMessage, [&w](quint32 instanceId, QByteArray) {
        qDebug() << "instanceId" << instanceId;
        raiseWidget(&w);
    });

    w.setWindowIcon(QIcon(":/res/icon.ico"));
    w.show();
    return a.exec();
}

void raiseWidget(MainWindow* widget)
{
#ifdef Q_OS_WINDOWS
    HWND hwnd = (HWND)widget->winId();

    /// Check if window is minimized to task bar
    if(::IsIconic(hwnd))
    {
        ::ShowWindow(hwnd, SW_RESTORE);
    }
    ::SetForegroundWindow(hwnd);
#else
    widget->show();
    widget->raise();
    widget->activateWindow();
#endif
}

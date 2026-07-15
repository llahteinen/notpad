#include "notpad.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>
#include <SingleApplication>

#ifdef Q_OS_WINDOWS
#include <Windows.h>
#endif


void raiseWidget(QWidget* widget);

int main(int argc, char *argv[])
{
    /// Single instance for this application
#ifdef Q_OS_WINDOWS
    SingleApplication a(argc, argv, true, /// Allow secondary instances
                        (SingleApplication::Mode::User |
                         SingleApplication::Mode::SecondaryNotification)
                        );

    if(a.isSecondary())
    {
        /// Enable the primary instance to set itself as foreground window
        AllowSetForegroundWindow( DWORD( a.primaryPid() ) );

        /// This secondary instance sends a message to primary instance
        a.sendMessage("INSTANCE_STARTED");
        return 0;
    }
#else
    /// This emits the instanceStarted signal normally
    SingleApplication a(argc, argv);
#endif

    QObject::connect(&a, &SingleApplication::instanceStarted,
                     [](){ qInfo() << "New instance"; } );


    QCommandLineParser parser;
//    parser.setApplicationDescription(PROJECT_NAME);
    parser.addHelpOption();
    parser.addVersionOption();

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

    NotPad w{parser};

#ifdef Q_OS_WINDOWS
    QObject::connect(&a, &SingleApplication::receivedMessage, [&w]() {
        raiseWidget(&w);
    });
#else
    QObject::connect(&a, &SingleApplication::instanceStarted, [&w]() {
        raiseWidget(&w);
    });
#endif

    w.setWindowIcon(QIcon(":/res/icon.ico"));
    w.show();
    return a.exec();
}

void raiseWidget(QWidget* widget)
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

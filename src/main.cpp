#include "notpad.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

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
    w.setWindowIcon(QIcon(":/res/icon.ico"));
    w.show();
    return a.exec();
}

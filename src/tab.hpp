#ifndef TAB_H
#define TAB_H

#include <QObject>

class QTabWidget;
class QPlainTextEdit;


class Tab
{
public:
    Tab(QPlainTextEdit* plainEditorTemplate)
        : m_plainEditorTemplate{plainEditorTemplate}
    {}

    QWidget* createEmptyTab();

private:
    QPlainTextEdit* m_plainEditorTemplate;
};

class TabManager : public QObject
{
    Q_OBJECT
public:
    TabManager(QTabWidget* tabWidget, QPlainTextEdit* plainEditorTemplate, QObject* parent = nullptr);

    void addTabFromFile(const QString& filePath, const QString& title = "");

    void closeCurrentTab();

public slots:

    void addEmptyTab();
    void onTabCloseRequested(int index);

private:
    QTabWidget* const m_tabWidget;
    Tab m_factory;
};


/*
class CustomTabBar : public QTabBar {
    Q_OBJECT
public:
    CustomTabBar(QWidget* parent = nullptr) : QTabBar(parent) {}

signals:
    void emptySpaceDoubleClicked();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override {
        // Check if the double-click is on empty space
        if (tabAt(event->pos()) == -1) {
            emit emptySpaceDoubleClicked();
        } else {
            QTabBar::mouseDoubleClickEvent(event);
        }
    }
};
*/

#endif // TAB_H

#ifndef TAB_HPP
#define TAB_HPP

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
    void closeTab(int index);
    QWidget* currentWidget() const;

public slots:

    void addEmptyTab();
    void onTabCloseRequested(int index);
    void onTabBarDoubleClicked(int index);
    void onCurrentChanged(int index);

private:
    QTabWidget* const m_tabWidget;
    Tab m_factory;

signals:
    void tabCloseRequested(int index);
    void currentChanged(int index);

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

#endif // TAB_HPP

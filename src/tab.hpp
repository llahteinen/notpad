#ifndef TAB_HPP
#define TAB_HPP

#include "file.hpp"
#include <QTabWidget>

class Editor;


class Tab
{
    friend class TabManager;
private:
    Tab() {}

    Editor* createEmptyTab();
    Editor* createTabFromFile(File::Status& o_status, const QString& fileName);
    void setupEditor(Editor* editor);

};

class TabManager : public QTabWidget
{
    Q_OBJECT
public:
    TabManager(QWidget* parent = nullptr);

    void setupUi();

    File::Status addTabFromFile(const QString& fileName);

    void closeTab(int index);
    void resetTab(int index);
    void updateTabText(const Editor* editor);

    using QTabWidget::addTab;

public slots:

    void addEmptyTab();
    void onTabBarDoubleClicked(int index);
    void onNameChanged(const QString& new_name);
    void onModificationChanged(bool modified);

private:
    /// \brief Adds a tab and sets it active
    void addTab(Editor* editor);

    Tab m_factory;

signals:

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

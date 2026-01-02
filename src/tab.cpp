#include "tab.hpp"
#include "editor.hpp"
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTabBar>
#include <QToolButton>
#include <QLabel>
#include <QLayout>


Editor* Tab::createEmptyTab()
{
    auto* editor = new Editor();
    setupEditor(editor, m_plainEditorTemplate);
    return editor;
}

Editor* Tab::createTabFromFile(File::Status& o_status, const QString& fileName)
{
    auto* editor = Editor::createEditor(o_status, fileName);
    if(editor == nullptr)
    {
        return nullptr;
    }

    setupEditor(editor, m_plainEditorTemplate);
    return editor;
}

void Tab::setupEditor(Editor* editor, const QPlainTextEdit* const templ)
{
    /// Basic settings
    editor->setUndoRedoEnabled(true);

    /// Dynamic global settings
    editor->setFont(templ->font());
    editor->setWordWrapMode(templ->wordWrapMode());
}

TabManager::TabManager(QTabWidget* tabWidget, QPlainTextEdit* plainEditorTemplate, QObject* parent)
        : QObject(parent)
        , m_tabWidget{tabWidget}
        , m_factory{plainEditorTemplate}
{
    m_tabWidget->tabBar()->setDrawBase(false); /// false ehkä ihan OK
    m_tabWidget->tabBar()->setDocumentMode(true);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &TabManager::onTabCloseRequested);
    connect(m_tabWidget, &QTabWidget::tabBarDoubleClicked, this, &TabManager::onTabBarDoubleClicked);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &TabManager::onCurrentChanged);

    QToolButton* tb = new QToolButton(m_tabWidget);
    tb->setText("+");
    tb->setFont(QFont("Consolas", 15));
//    tb->setArrowType(Qt::ArrowType::DownArrow); /// Ei.
//    tb->setAutoRaise(true);
    tb->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
//    tb->setIconSize({24,24}); /// Tekee blurria
//    tb->setMinimumWidth(26);

    connect(tb, &QToolButton::clicked, this, &TabManager::addEmptyTab);

//    const auto index = m_tabWidget->addTab(new QLabel(), QString());
//    m_tabWidget->tabBar()->setTabButton(index, QTabBar::RightSide, tb); /// QTabBar::RightSide will replace the default X button
//    m_tabWidget->setTabEnabled(index, false);

//    auto* layout = m_tabWidget->layout();
//    layout->setAlignment(tb, Qt::AlignRight);
//    m_tabWidget->tabBar()->layout()->addWidget(tb);
//    m_tabWidget->tabBar()->layout()->setAlignment(tb, Qt::AlignRight);

    m_tabWidget->setCornerWidget(tb, Qt::TopRightCorner);
}

void TabManager::addTab(QWidget* editor, const QString& title)
{
    const int index = m_tabWidget->addTab(editor, QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew), title);

//    /// Custom close button
//    QToolButton* tb = new QToolButton(m_tabWidget);
//    tb->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
//    m_tabWidget->tabBar()->setTabButton(index, QTabBar::RightSide, tb);
    /// Toimii mutta pitää tehdä signaalit

    m_tabWidget->setCurrentIndex(index);
}

void TabManager::addEmptyTab()
{
    QWidget* new_editor = m_factory.createEmptyTab();
    addTab(new_editor);
}

File::Status TabManager::addTabFromFile(const QString& fileName)
{
    File::Status status;
    Editor* editor = m_factory.createTabFromFile(status, fileName);
    if(editor == nullptr)
    {
        return status;
    }

    addTab(editor, QFileInfo(*editor->m_file).fileName());
    return status;
}

void TabManager::closeCurrentTab()
{
    int currentIndex = m_tabWidget->currentIndex();
    if(currentIndex >= 0)
    {
        onTabCloseRequested(currentIndex);
    }
}

void TabManager::closeTab(int index)
{
    qDebug() << "closeTab" << index;
    QWidget* tabContent = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    tabContent->deleteLater(); /// Not sure if this is needed
}

QWidget* TabManager::currentWidget() const
{
    return m_tabWidget->currentWidget();
}

/// Only fired by manually closing a tab
void TabManager::onTabCloseRequested(int index)
{
    qDebug() << "onTabCloseRequested" << index;
    /// Need to ask main for permission to close tab
    emit tabCloseRequested(index);
}

void TabManager::onTabBarDoubleClicked(int index)
{
    qDebug() << "onTabBarDoubleClicked" << index;
    if(index == -1)
    {
        addEmptyTab();
    }
}

/// This gets fired also when index 0 is closed and the new current index is again 0
void TabManager::onCurrentChanged(int index)
{
//    qDebug() << "onCurrentChanged" << index;
    emit currentChanged(index);
}

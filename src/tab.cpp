#include "tab.hpp"
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTabBar>
#include <QToolButton>
#include <QLabel>
#include <QLayout>


QWidget* Tab::createEmptyTab()
{
    auto* editor = new QPlainTextEdit();
    auto* templ = m_plainEditorTemplate;

    /// Basic settings
    editor->setUndoRedoEnabled(true);

    /// Dynamic global settings
    editor->setFont(templ->font());
    editor->setWordWrapMode(templ->wordWrapMode());

    return editor;
}

TabManager::TabManager(QTabWidget* tabWidget, QPlainTextEdit* plainEditorTemplate, QObject* parent)
        : QObject(parent)
        , m_tabWidget{tabWidget}
        , m_factory{plainEditorTemplate}
{
    m_tabWidget->tabBar()->setDrawBase(false); /// false ehkä ihan OK
    m_tabWidget->tabBar()->setDocumentMode(true);

    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &TabManager::onTabCloseRequested);

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

void TabManager::addEmptyTab()
{
    QWidget* new_editor = m_factory.createEmptyTab();
//    const int index = qMax(m_tabWidget->count(), 0);
//    m_tabWidget->insertTab(index, new_editor, "title");
    const int index = m_tabWidget->addTab(new_editor, "Untitled");
    m_tabWidget->setCurrentIndex(index);

//    QToolButton* tb = new QToolButton(m_tabWidget);
//    tb->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
//    m_tabWidget->tabBar()->setTabButton(index, QTabBar::RightSide, tb);
    /// Toimii mutta pitää tehdä signaalit
}

void TabManager::addTabFromFile(const QString& /*filePath*/, const QString& /*title*/)
{
//    QWidget* tabContent = m_factory.createTabFromFile(filePath);
//    QString tabTitle = title.isEmpty() ? QFileInfo(filePath).fileName() : title;
//    m_tabWidget->addTab(tabContent, tabTitle);
}

void TabManager::closeCurrentTab()
{
    int currentIndex = m_tabWidget->currentIndex();
    if(currentIndex >= 0)
    {
        onTabCloseRequested(currentIndex);
    }
}

void TabManager::onTabCloseRequested(int index)
{
    QWidget* tabContent = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    delete tabContent; /// En tiä tarvitaanko
}

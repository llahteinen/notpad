#include "tab.hpp"
#include "editor.hpp"
#include "settings.hpp"
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
    Q_UNUSED(templ);
    /// Basic settings
    editor->setUndoRedoEnabled(true);

    /// Dynamic global settings
    editor->setFont(SETTINGS.font);
    editor->setWordWrap(SETTINGS.wordWrap);
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

void TabManager::addTab(Editor* editor)
{
    const int index = m_tabWidget->addTab(editor, QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew), editor->name());

    connect(editor, &Editor::nameChanged, this, &TabManager::onNameChanged, Qt::UniqueConnection);
    connect(editor, &Editor::modificationChanged, this, &TabManager::onModificationChanged, Qt::UniqueConnection);

//    /// Custom close button
//    QToolButton* tb = new QToolButton(m_tabWidget);
//    tb->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
//    m_tabWidget->tabBar()->setTabButton(index, QTabBar::RightSide, tb);
    /// Toimii mutta pitää tehdä signaalit

    m_tabWidget->setCurrentIndex(index);
}

void TabManager::addEmptyTab()
{
    Editor* new_editor = m_factory.createEmptyTab();
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

    addTab(editor);
    return status;
}

int TabManager::count() const
{
    return m_tabWidget->count();
}

int TabManager::currentIndex() const
{
    return m_tabWidget->currentIndex();
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

void TabManager::resetTab(int index)
{
    qDebug() << "resetTab" << index;
    /// We want to emit signals about changed tabs only once
    const auto blocked = m_tabWidget->blockSignals(true);
    closeTab(index);
    m_tabWidget->blockSignals(blocked);
    addEmptyTab();
}

QWidget* TabManager::currentWidget() const
{
    return m_tabWidget->currentWidget();
}

void TabManager::updateTabText(const Editor* editor)
{
    qDebug() << "updateTabText" << editor->name();
    const bool modified = editor->isModified();
    const QString text = QString("%1%2").arg((modified ? "*" : ""), editor->name());
    m_tabWidget->setTabText(m_tabWidget->indexOf(editor), text);
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

void TabManager::onNameChanged([[maybe_unused]] const QString& new_name)
{
//    qDebug() << "TabManager::onNameChanged";
    const auto editor = qobject_cast<const Editor*>(sender());
    Q_ASSERT(editor != nullptr);
    if(editor != nullptr)
    {
        updateTabText(editor);
    }
    else
    {
        qDebug() << "editor is null";
    }
}

void TabManager::onModificationChanged([[maybe_unused]] bool modified)
{
//    qDebug() << "TabManager::onModificationChanged" << modified << sender();
    const auto editor = qobject_cast<const Editor*>(sender());
    Q_ASSERT(editor != nullptr);
    if(editor != nullptr)
    {
        updateTabText(editor);
    }
    else
    {
        qDebug() << "editor is null";
    }
}

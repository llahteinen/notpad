#include "tab.hpp"
#include "editor.hpp"
#include "settings.hpp"
#include <QTabBar>
#include <QToolButton>


Editor* TabManager::createEmptyEditor()
{
    auto* editor = new Editor();
    setupEditor(editor);
    return editor;
}

Editor* TabManager::createEditorFromFile(File::Status& o_status, const QString& fileName)
{
    auto* editor = Editor::createEditor(o_status, fileName);
    if(editor == nullptr)
    {
        return nullptr;
    }

    setupEditor(editor);
    return editor;
}

void TabManager::setupEditor(Editor* editor)
{
    /// Basic settings
    editor->setUndoRedoEnabled(true);

    /// Dynamic global settings
    auto font = SETTINGS.font;
    font.setPointSize(SETTINGS.pers.zoomFontSize);
    editor->setFont(font);
    editor->setWordWrap(SETTINGS.pers.wordWrap);
}

TabManager::TabManager(QWidget* parent)
        : QTabWidget(parent)
{
    connect(this, &QTabWidget::tabBarDoubleClicked, this, &TabManager::onTabBarDoubleClicked);
}

void TabManager::setupUi()
{
    tabBar()->setDrawBase(false); /// false ehkä ihan OK
    tabBar()->setDocumentMode(true);

    QToolButton* tb = new QToolButton(this);
    tb->setText("+");
    tb->setFont(QFont("Consolas", 15));
//    tb->setArrowType(Qt::ArrowType::DownArrow); /// Ei.
//    tb->setAutoRaise(true);
    tb->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
//    tb->setIconSize({24,24}); /// Tekee blurria
//    tb->setMinimumWidth(26);

    connect(tb, &QToolButton::clicked, this, &TabManager::addEmptyTab);

    QTabWidget::setCornerWidget(tb, Qt::TopRightCorner);
}

void TabManager::addTab(Editor* editor)
{
    const int index = QTabWidget::addTab(editor, QIcon::fromTheme(QIcon::ThemeIcon::DocumentNew), editor->name());

    connect(editor, &Editor::nameChanged, this, &TabManager::onNameChanged, Qt::UniqueConnection);
    connect(editor, &Editor::modificationChanged, this, &TabManager::onModificationChanged, Qt::UniqueConnection);

//    /// Custom close button
//    QToolButton* tb = new QToolButton(this);
//    tb->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::EditClear));
//    tabBar()->setTabButton(index, QTabBar::RightSide, tb);
    /// Toimii mutta pitää tehdä signaalit

    QTabWidget::setCurrentIndex(index);
}

void TabManager::addEmptyTab()
{
    Editor* new_editor = createEmptyEditor();
    addTab(new_editor);
}

File::Status TabManager::addTabFromFile(const QString& fileName)
{
    File::Status status;
    Editor* editor = createEditorFromFile(status, fileName);
    if(editor == nullptr)
    {
        return status;
    }

    addTab(editor);
    return status;
}

void TabManager::closeTab(int index)
{
    qDebug() << "closeTab" << index;
    QWidget* tabContent = QTabWidget::widget(index);
    QTabWidget::removeTab(index);
    tabContent->deleteLater(); /// Not sure if this is needed
}

void TabManager::resetTab(int index)
{
    qDebug() << "resetTab" << index;
    /// We want to emit signals about changed tabs only once
    const auto blocked = blockSignals(true);
    closeTab(index);
    blockSignals(blocked);
    addEmptyTab();
}

void TabManager::updateTabText(const Editor* editor)
{
    qDebug() << "updateTabText" << editor->name();
    const bool modified = editor->isModified();
    const QString text = QString("%1%2").arg((modified ? "*" : ""), editor->name());
    QTabWidget::setTabText(QTabWidget::indexOf(editor), text);
}

Editor* TabManager::currentWidget() const
{
    auto* widget = QTabWidget::currentWidget();
    qDebug() << "widget" << widget;
    return qobject_cast<Editor*>(widget);
}

Editor* TabManager::widget(int index) const
{
    auto* widget = QTabWidget::widget(index);
    qDebug() << "widget" << widget;
    auto* editor = qobject_cast<Editor*>(widget);
    return editor;
}

void TabManager::onTabBarDoubleClicked(int index)
{
    qDebug() << "onTabBarDoubleClicked" << index;
    if(index == -1)
    {
        addEmptyTab();
    }
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

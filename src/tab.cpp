#include "tab.hpp"
#include "editor.hpp"
#include "settings.hpp"
#include <QTabBar>


Editor* TabManager::createEmptyEditor()
{
    auto* editor = new Editor(); /// Ownership goes later to QTabWidget::addTab
    setupEditor(editor);
    return editor;
}

Editor* TabManager::createEditorFromFile(File::Status& o_status, const QString& fileName)
{
    auto* editor = Editor::createEditor(o_status, fileName); /// Ownership goes later to QTabWidget::addTab
    if(editor == nullptr)
    {
        return nullptr;
    }

    setupEditor(editor);
    return editor;
}

/// NOTE: Only do things here that can be done before the editor text loading is finished
void TabManager::setupEditor(Editor* editor)
{
    /// Dynamic global settings
    auto font = SETTINGS.font;
    font.setPointSize(SETTINGS.pers.zoomFontSize);
    editor->setFont(font);
    editor->setWordWrap(SETTINGS.pers.wordWrap);

    /// Hard coded stuff
    /// Setting this to false will let dragEnterEvent etc pass through to the underneath widget / main window
    editor->setAcceptDrops(false);
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
}

void TabManager::addTab(Editor* editor)
{
    /// Windows doesn't have "document" but for example Ubuntu has
    const int index = QTabWidget::addTab(editor, "");
    updateTabTitle(editor);

    connect(editor, &Editor::nameChanged, this, &TabManager::onNameChanged, Qt::UniqueConnection);
    connect(editor, &Editor::modificationChanged, this, &TabManager::onModificationChanged, Qt::UniqueConnection);

//    /// Custom close button
//    QToolButton* tb = new QToolButton(this);
//    tb->setIcon(QIcon::fromTheme("edit-clear"));
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
    QTabWidget::removeTab(index); /// "The page widget itself is not deleted."
    tabContent->deleteLater(); /// Delete tab content
}

void TabManager::closeCurrentTab()
{
    closeTab(currentIndex());
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

void TabManager::updateTabTitle(const Editor* editor)
{
    qDebug() << "updateTabTitle" << editor->name();
    const bool modified = editor->isModified();

//    const QString text = QString("%1%2").arg((modified ? "*" : ""), editor->name()); /// Highlight modified with asterisk
    const QString text = QString("%2").arg(editor->name()); /// Just the name - highlight modified with icon only
    const QIcon icon = modified ?
        QIcon::fromTheme("document-save-as") :
        QIcon::fromTheme("document-new");
    const QString tooltip = QString("%1%2").arg(editor->filePath(), (modified ? " (modified)" : ""));

    const auto index = QTabWidget::indexOf(editor);
    QTabWidget::setTabText(index, text);
    QTabWidget::setTabIcon(index, icon);
    QTabWidget::setTabToolTip(index, tooltip);
}

Editor* TabManager::currentWidget() const
{
    auto* widget = QTabWidget::currentWidget();
    qDebug() << "currentWidget" << widget;
    return qobject_cast<Editor*>(widget);
}

Editor* TabManager::widget(int index) const
{
    auto* widget = QTabWidget::widget(index);
    qDebug() << "widget" << widget;
    auto* editor = qobject_cast<Editor*>(widget);
    return editor;
}

void TabManager::iterateTabs(std::function<bool(Editor* editor)> processor)
{
    Q_ASSERT(processor);
    for(auto* editor : getProcessingOrder())
    {
        const auto abort = processor(editor);
        if(abort) break;
    }
}

QList<Editor*> TabManager::getProcessingOrder()
{
    if(count() <= 0 || currentIndex() < 0)
    {
        return {};
    }

    QList<Editor*> tabs;

    const auto add = [this, &tabs](int i) {
        auto* editor = widget(i);
        Q_ASSERT(editor != nullptr);
        if(!tabs.contains(editor)) { tabs.append(editor); }
    };

    /// Similar like QTabWidget uses when selecting the active tab after closing tabs with QTabBar::SelectRightTab
    /// Current tab first, then rightward till the end, and
    /// lastly leftward from the original current tab
    const auto current = currentIndex();
    const auto last = count() - 1;
    for(int i = current; i <= last; ++i)
    {
        add(i);
    }
    for(int i = current - 1; i >= 0; --i)
    {
        add(i);
    }

//    /// Current tab first, then rest of them backwards starting from end
//    {
//        add(currentIndex());
//        const auto last = count() - 1;
//        for(int i = last; i >= 0; --i)
//        {
//            add(i);
//        }
//    }

    return tabs;
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
        updateTabTitle(editor);
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
        updateTabTitle(editor);
    }
    else
    {
        qDebug() << "editor is null";
    }
}

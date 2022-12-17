#include "ExternalResourcesPage.h"
#include "ui_ExternalResourcesPage.h"

#include "DesktopServices.h"
#include "Version.h"
#include "minecraft/mod/ResourceFolderModel.h"
#include "ui/GuiUtil.h"

#include <QKeyEvent>
#include <QMenu>

ExternalResourcesPage::ExternalResourcesPage(BaseInstance* instance, std::shared_ptr<ResourceFolderModel> model, QWidget* parent)
    : QMainWindow(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance(instance), ui(new Ui::ExternalResourcesPage), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model(model)
{
    ui->setupUi(this);

    ui->actionsToolbar->insertSpacer(ui->actionViewConfigs);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel = model->createFilterProxyModel(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->setDynamicSortFilter(true);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->setSourceModel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model.get());
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->setFilterKeyColumn(-1);
    ui->treeView->setModel(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel);

    ui->treeView->installEventFilter(this);
    ui->treeView->sortByColumn(1, Qt::AscendingOrder);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);

    // The default function names by Qt are pretty ugly, so let's just connect the actions manually,
    // to make it easier to read :)
    connect(ui->actionAddItem, &QAction::triggered, this, &ExternalResourcesPage::addItem);
    connect(ui->actionRemoveItem, &QAction::triggered, this, &ExternalResourcesPage::removeItem);
    connect(ui->actionEnableItem, &QAction::triggered, this, &ExternalResourcesPage::enableItem);
    connect(ui->actionDisableItem, &QAction::triggered, this, &ExternalResourcesPage::disableItem);
    connect(ui->actionViewConfigs, &QAction::triggered, this, &ExternalResourcesPage::viewConfigs);
    connect(ui->actionViewFolder, &QAction::triggered, this, &ExternalResourcesPage::viewFolder);

    connect(ui->treeView, &ModListView::customContextMenuRequested, this, &ExternalResourcesPage::ShowContextMenu);
    connect(ui->treeView, &ModListView::activated, this, &ExternalResourcesPage::itemActivated);

    auto selection_model = ui->treeView->selectionModel();
    connect(selection_model, &QItemSelectionModel::currentChanged, this, &ExternalResourcesPage::current);
    connect(ui->filterEdit, &QLineEdit::textChanged, this, &ExternalResourcesPage::filterTextChanged);
}

ExternalResourcesPage::~ExternalResourcesPage()
{
    delete ui;
}

QMenu* ExternalResourcesPage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->actionsToolbar->toggleViewAction());
    return filteredMenu;
}

void ExternalResourcesPage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->actionsToolbar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->treeView->mapToGlobal(pos));
    delete menu;
}

void ExternalResourcesPage::openedImpl()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->startWatching();

    auto const setting_name = QString("WideBarVisibility_%1").arg(id());
    if (!APPLICATION->settings()->contains(setting_name))
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_wide_bar_setting = APPLICATION->settings()->registerSetting(setting_name);
    else
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_wide_bar_setting = APPLICATION->settings()->getSetting(setting_name);

    ui->actionsToolbar->setVisibilityState(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_wide_bar_setting->get().toByteArray());
}

void ExternalResourcesPage::closedImpl()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->stopWatching();

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_wide_bar_setting->set(ui->actionsToolbar->getVisibilityState());
}

void ExternalResourcesPage::retranslate()
{
    ui->retranslateUi(this);
}

void ExternalResourcesPage::itemActivated(const QModelIndex&)
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_controlsEnabled)
        return;

    auto selection = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->setResourceEnabled(selection.indexes(), EnableAction::TOGGLE);
}

void ExternalResourcesPage::filterTextChanged(const QString& newContents)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_viewFilter = newContents;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->setFilterRegularExpression(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_viewFilter);
}

bool ExternalResourcesPage::shouldDisplay() const
{
    return true;
}

bool ExternalResourcesPage::listFilter(QKeyEvent* keyEvent)
{
    switch (keyEvent->key()) {
        case Qt::Key_Delete:
            removeItem();
            return true;
        case Qt::Key_Plus:
            addItem();
            return true;
        default:
            break;
    }
    return QWidget::eventFilter(ui->treeView, keyEvent);
}

bool ExternalResourcesPage::eventFilter(QObject* obj, QEvent* ev)
{
    if (ev->type() != QEvent::KeyPress)
        return QWidget::eventFilter(obj, ev);
    
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(ev);
    if (obj == ui->treeView)
        return listFilter(keyEvent);

    return QWidget::eventFilter(obj, ev);
}

void ExternalResourcesPage::addItem()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_controlsEnabled)
        return;
    

    auto list = GuiUtil::BrowseForFiles(
        helpPage(), tr("Select %1", "Select whatever type of files the page contains. Example: 'Loader Mods'").arg(displayName()),
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fileSelectionFilter.arg(displayName()), APPLICATION->settings()->get("CentralModsDir").toString(), this->parentWidget());

    if (!list.isEmpty()) {
        for (auto filename : list) {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->installResource(filename);
        }
    }
}

void ExternalResourcesPage::removeItem()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_controlsEnabled)
        return;
    
    auto selection = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->deleteResources(selection.indexes());
}

void ExternalResourcesPage::enableItem()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_controlsEnabled)
        return;

    auto selection = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->setResourceEnabled(selection.indexes(), EnableAction::ENABLE);
}

void ExternalResourcesPage::disableItem()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_controlsEnabled)
        return;

    auto selection = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->mapSelectionToSource(ui->treeView->selectionModel()->selection());
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->setResourceEnabled(selection.indexes(), EnableAction::DISABLE);
}

void ExternalResourcesPage::viewConfigs()
{
    DesktopServices::openDirectory(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance->instanceConfigFolder(), true);
}

void ExternalResourcesPage::viewFolder()
{
    DesktopServices::openDirectory(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->dir().absolutePath(), true);
}

bool ExternalResourcesPage::current(const QModelIndex& current, const QModelIndex& previous)
{
    if (!current.isValid()) {
        ui->frame->clear();
        return false;
    }

    return onSelectionChanged(current, previous);
}

bool ExternalResourcesPage::onSelectionChanged(const QModelIndex& current, const QModelIndex& previous)
{
    auto sourceCurrent = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filterModel->mapToSource(current);
    int row = sourceCurrent.row();
    Resource const& resource = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model->at(row);
    ui->frame->updateWithResource(resource);

    return true;
}


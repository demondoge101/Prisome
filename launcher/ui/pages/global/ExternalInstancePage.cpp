// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 初夏同学 <2411829240@qq.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ExternalInstancePage.h"
#include "ui_ExternalInstancePage.h"

#include <QMenu>
#include <QStringListModel>

#include <QFileDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStyledItemDelegate>

#include "Application.h"
#include "BuildConfig.h"
#include "DesktopServices.h"
#include "FileSystem.h"
#include "InstanceList.h"
class EditWidget : public QWidget {
    Q_OBJECT
   public:
    explicit EditWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags()) : QWidget(parent, f)
    {
        auto* layout = new QHBoxLayout(this);
        m_lineEdit = new QLineEdit(this);
        m_lineEdit->installEventFilter(const_cast<EditWidget*>(this));
        auto* pushButton = new QPushButton(tr("Open"), this);
        connect(pushButton, &QPushButton::clicked, [=]() {
            QString raw_dir = QFileDialog::getExistingDirectory(pushButton, tr("External Instance Folder"), m_lineEdit->text());
            if (!raw_dir.isEmpty())
                m_lineEdit->setText(raw_dir);
        });

        layout->addWidget(m_lineEdit);
        layout->addWidget(pushButton);

        layout->setContentsMargins(0, 0, 0, 0);
        this->setLayout(layout);
    };

    ~EditWidget() override = default;
    bool eventFilter(QObject* obj, QEvent* event) override
    {
        if (event->type() == QEvent::KeyPress) {
            auto* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
                emit editDone();
                return true;
            }
        }
        return false;
    }

    inline void setData(const QString& data) { m_lineEdit->setText(data); }
    inline QString getData() { return m_lineEdit->text(); }

   signals:
    void editDone();

   private:
    QLineEdit* m_lineEdit;
};

class FolderButtonDelegate : public QStyledItemDelegate {
    Q_OBJECT
   public:
    explicit FolderButtonDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        Q_UNUSED(index);
        Q_UNUSED(option);
        auto* editorWidget = new EditWidget(parent);
        connect(editorWidget, &EditWidget::editDone, this, &FolderButtonDelegate::editDone);

        return editorWidget;
    }

    void setEditorData(QWidget* editor, const QModelIndex& index) const override
    {
        auto text = index.data(Qt::EditRole).toString();
        qobject_cast<EditWidget*>(editor)->setData(text);
    }
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
    {
        auto text = qobject_cast<EditWidget*>(editor)->getData();
        if (ExternalInstancePage::verifyInstDirPath(text)) {
            QString cooked_dir = FS::NormalizePath(text);
            if (!dynamic_cast<QStringListModel*>(model)->stringList().contains(cooked_dir))
                model->setData(index, cooked_dir, Qt::EditRole);
        }
    }
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        Q_UNUSED(index);
        editor->setGeometry(option.rect);
    }

   public slots:
    void editDone()
    {
        auto* editor = qobject_cast<EditWidget*>(sender());
        emit commitData(editor);
        emit closeEditor(editor);
    };
};

ExternalInstancePage::ExternalInstancePage(QWidget* parent) : QMainWindow(parent), ui(new Ui::ExternalInstancePage)
{
    ui->setupUi(this);
    ui->listView->setEmptyString(
        tr("Welcome!\n"
           "You can add external instance directories here. (The directory where the instance folder is stored, not the instance folder "
           "itself.)"));
    ui->listView->setEmptyMode(VersionListView::String);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->listView->header()->setSectionResizeMode(QHeaderView::Stretch);
    ui->listView->setHeaderHidden(true);
    ui->listView->setEditTriggers(QAbstractItemView::DoubleClicked);

    m_model = new QStringListModel(this);

    ui->listView->setModel(m_model);

    ui->listView->setItemDelegate(new FolderButtonDelegate(ui->listView));
    connect(ui->listView, &VersionListView::customContextMenuRequested, this, &ExternalInstancePage::ShowContextMenu);
}

ExternalInstancePage::~ExternalInstancePage()
{
    delete ui;
}

void ExternalInstancePage::retranslate()
{
    ui->retranslateUi(this);
}

void ExternalInstancePage::ShowContextMenu(const QPoint& pos)
{
    auto menu = ui->toolBar->createContextMenu(this, tr("Context menu"));
    menu->exec(ui->listView->mapToGlobal(pos));
    delete menu;
}

void ExternalInstancePage::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    QMainWindow::changeEvent(event);
}

QMenu* ExternalInstancePage::createPopupMenu()
{
    QMenu* filteredMenu = QMainWindow::createPopupMenu();
    filteredMenu->removeAction(ui->toolBar->toggleViewAction());
    return filteredMenu;
}

inline bool ExternalInstancePage::verifyInstDirPath(const QString& raw_dir)
{
    bool result = false;
    if (!raw_dir.isEmpty() && QDir(raw_dir).exists()) {
        QString cooked_dir = FS::NormalizePath(raw_dir);
        if (FS::checkProblemticPathJava(QDir(cooked_dir))) {
            QMessageBox warning;
            warning.setText(
                tr("You're trying to specify an instance folder which\'s path "
                   "contains at least one \'!\'. "
                   "Java is known to cause problems if that is the case, your "
                   "instances (probably) won't start!"));
            warning.setInformativeText(
                tr("Do you really want to use this path? "
                   "Selecting \"No\" will close this and not alter your instance path."));
            warning.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            int result1 = warning.exec();
            if (result1 == QMessageBox::Ok) {
                result = true;
            }
        } else if (APPLICATION->settings()->get("InstanceDir").toString() == cooked_dir) {
            QMessageBox warning;
            warning.setText(
                tr("The external instance directory cannot be set to the root directory!").arg(BuildConfig.LAUNCHER_DISPLAYNAME));
            warning.setStandardButtons(QMessageBox::Cancel);
            warning.exec();
        } else if (DesktopServices::isFlatpak() && raw_dir.startsWith("/run/user")) {
            QMessageBox warning;
            warning.setText(tr("You're trying to specify an instance folder "
                               "which was granted temporarily via Flatpak.\n"
                               "This is known to cause problems. "
                               "After a restart the launcher might break, "
                               "because it will no longer have access to that directory.\n\n"
                               "Granting %1 access to it via Flatseal is recommended.")
                                .arg(BuildConfig.LAUNCHER_DISPLAYNAME));
            warning.setInformativeText(tr("Do you want to proceed anyway?"));
            warning.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
            int result1 = warning.exec();
            if (result1 == QMessageBox::Ok) {
                result = true;
            }
        } else {
            result = true;
        }
    }
    return result;
}

void ExternalInstancePage::on_actionAddExtInst_triggered()
{
    QString raw_dir = QFileDialog::getExistingDirectory(this, tr("External Instance Folder"));

    if (verifyInstDirPath(raw_dir)) {
        QString cooked_dir = FS::NormalizePath(raw_dir);
        m_model->insertRow(m_model->rowCount());
        auto index = m_model->index(m_model->rowCount() - 1, 0);
        m_model->setData(index, cooked_dir, Qt::DisplayRole);
        ui->listView->setCurrentIndex(index);
    }
}

void ExternalInstancePage::on_actionRemove_triggered()
{
    auto index = ui->listView->currentIndex();

    m_model->removeRow(index.row());
}

void ExternalInstancePage::on_actionHide_triggered() {}
bool ExternalInstancePage::apply()
{
    if (m_rootInstDir == APPLICATION->settings()->get("InstanceDir").toString())
        APPLICATION->instances()->setExtInstDir(m_model->stringList());

    return true;
}
void ExternalInstancePage::openedImpl()
{
    m_rootInstDir = APPLICATION->settings()->get("InstanceDir").toString();
    m_model->setStringList(APPLICATION->instances()->getExtInstDir());
}
#include "ExternalInstancePage.moc"
// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 Jamie Mansfield <jmansfield@cadixdev.org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <QWidget>
#include <QModelIndex>

#include "ui/pages/BasePageProvider.h"
#include "ui/pages/BasePageContainer.h"

class QLayout;
class IconLabel;
class QSortFilterProxyModel;
class PageModel;
class QLabel;
class QListView;
class QLineEdit;
class QStackedLayout;
class QGridLayout;

class PageContainer : public QWidget, public BasePageContainer
{
    Q_OBJECT
public:
    explicit PageContainer(BasePageProvider *pageProvider, QString defaultId = QString(),
                        QWidget *parent = 0);
    virtual ~PageContainer() {}

    void addButtons(QWidget * buttons);
    void addButtons(QLayout * buttons);
    /*
     * Save any unsaved state and prepare to be closed.
     * @return true if everything can be saved, false if there is something that requires attention
     */
    bool prepareToClose();
    bool saveAll();

    /* request close - used by individual pages */
    bool requestClose() override
    {
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container)
        {
            return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container->requestClose();
        }
        return false;
    }

    virtual bool selectPage(QString pageId) override;
    BasePage* getPage(QString pageId) override;

    void refreshContainer() override;
    virtual void setParentContainer(BasePageContainer * container)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container = container;
    };

    void changeEvent(QEvent*) override;

private:
    void createUI();
    void retranslate();

public slots:
    void help();

signals:
    /** Emitted when the currently selected page is changed */
    void selectedPageChanged(BasePage* previous, BasePage* selected);

private slots:
    void currentChanged(const QModelIndex &current);
    void showPage(int row);

private:
    BasePageContainer * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_container = nullptr;
    BasePage * hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_currentPage = 0;
    QSortFilterProxyModel *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_proxyModel;
    PageModel *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model;
    QStackedLayout *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageStack;
    QListView *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pageList;
    QLabel *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_header;
    IconLabel *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_iconHeader;
    QGridLayout *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_layout;
};

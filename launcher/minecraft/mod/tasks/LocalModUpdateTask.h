// SPDX-License-Identifier: GPL-3.0-only
/*
*  PolyMC - Minecraft Launcher
*  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#pragma once

#include <QDir>

#include "modplatform/ModIndex.h"
#include "tasks/Task.h"

class LocalModUpdateTask : public Task {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<LocalModUpdateTask>;

    explicit LocalModUpdateTask(QDir index_dir, ModPlatform::IndexedPack& mod, ModPlatform::IndexedVersion& mod_version);

    auto canAbort() const -> bool override { return true; }
    auto abort() -> bool override;

   protected slots:
    //! Entry point for tasks.
    void executeTask() override;

   signals:
    void hasOldMod(QString name, QString filename);

   private:
    QDir hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir;
    ModPlatform::IndexedPack& hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod;
    ModPlatform::IndexedVersion& hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_version;
};

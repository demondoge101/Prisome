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

#include "FileSink.h"

#include "FileSystem.h"

namespace Net {

Task::State FileSink::init(QNetworkRequest& request)
{
    auto result = initCache(request);
    if (result != Task::State::Running) {
        return result;
    }

    // create a new save file and open it for writing
    if (!FS::ensureFilePathExists(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename)) {
        qCritical() << "Could not create folder for " + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename;
        return Task::State::Failed;
    }

    wroteAnyData = false;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output_file.reset(new QSaveFile(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename));
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output_file->open(QIODevice::WriteOnly)) {
        qCritical() << "Could not open " + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename + " for writing";
        return Task::State::Failed;
    }

    if (initAllValidators(request))
        return Task::State::Running;
    return Task::State::Failed;
}

Task::State FileSink::write(QByteArray& data)
{
    if (!writeAllValidators(data) || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output_file->write(data) != data.size()) {
        qCritical() << "Failed writing into " + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output_file->cancelWriting();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output_file.reset();
        wroteAnyData = false;
        return Task::State::Failed;
    }

    wroteAnyData = true;
    return Task::State::Running;
}

Task::State FileSink::abort()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output_file->cancelWriting();
    failAllValidators();
    return Task::State::Failed;
}

Task::State FileSink::finalize(QNetworkReply& reply)
{
    bool gotFile = false;
    QVariant statusCodeV = reply.attribute(QNetworkRequest::HttpStatusCodeAttribute);
    bool validStatus = false;
    int statusCode = statusCodeV.toInt(&validStatus);
    if (validStatus) {
        // this leaves out 304 Not Modified
        gotFile = statusCode == 200 || statusCode == 203;
    }

    // if we wrote any data to the save file, we try to commit the data to the real file.
    // if it actually got a proper file, we write it even if it was empty
    if (gotFile || wroteAnyData) {
        // ask validators for data consistency
        // we only do this for actual downloads, not 'your data is still the same' cache hits
        if (!finalizeAllValidators(reply))
            return Task::State::Failed;

        // nothing went wrong...
        if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output_file->commit()) {
            qCritical() << "Failed to commit changes to " << hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename;
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output_file->cancelWriting();
            return Task::State::Failed;
        }
    }

    // then get rid of the save file
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_output_file.reset();

    return finalizeCache(reply);
}

Task::State FileSink::initCache(QNetworkRequest&)
{
    return Task::State::Running;
}

Task::State FileSink::finalizeCache(QNetworkReply&)
{
    return Task::State::Succeeded;
}

bool FileSink::hasLocalData()
{
    QFileInfo info(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_filename);
    return info.exists() && info.size() != 0;
}
}  // namespace Net

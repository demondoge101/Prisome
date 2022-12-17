// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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

#include <meta/VersionList.h>
#include <meta/Index.h>
#include "Component.h"

#include <QSaveFile>

#include "meta/Version.h"
#include "VersionFile.h"
#include "minecraft/PackProfile.h"
#include "FileSystem.h"
#include "OneSixVersionFormat.h"
#include "Application.h"

#include <assert.h>

Component::Component(PackProfile * parent, const QString& uid)
{
    assert(parent);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent = parent;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid = uid;
}

Component::Component(PackProfile * parent, std::shared_ptr<Meta::Version> version)
{
    assert(parent);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent = parent;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion = version;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid = version->uid();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVersion = version->version();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedName = version->name();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loaded = version->isLoaded();
}

Component::Component(PackProfile * parent, const QString& uid, std::shared_ptr<VersionFile> file)
{
    assert(parent);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent = parent;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file = file;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid = uid;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVersion = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file->version;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedName = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file->name;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loaded = true;
}

std::shared_ptr<Meta::Version> Component::getMeta()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion;
}

void Component::applyTo(LaunchProfile* profile)
{
    // do not apply disabled components
    if(!isEnabled())
    {
        return;
    }
    auto vfile = getVersionFile();
    if(vfile)
    {
        vfile->applyTo(profile, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->runtimeContext());
    }
    else
    {
        profile->applyProblemSeverity(getProblemSeverity());
    }
}

std::shared_ptr<class VersionFile> Component::getVersionFile() const
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion)
    {
        if(!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion->isLoaded())
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion->load(Net::Mode::Online);
        }
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion->data();
    }
    else
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file;
    }
}

std::shared_ptr<class Meta::VersionList> Component::getVersionList() const
{
    // FIXME: what if the metadata index isn't loaded yet?
    if(APPLICATION->metadataIndex()->hasUid(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid))
    {
        return APPLICATION->metadataIndex()->get(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid);
    }
    return nullptr;
}

int Component::getOrder()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_orderOverride)
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_order;

    auto vfile = getVersionFile();
    if(vfile)
    {
        return vfile->order;
    }
    return 0;
}
void Component::setOrder(int order)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_orderOverride = true;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_order = order;
}
QString Component::getID()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid;
}
QString Component::getName()
{
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedName.isEmpty())
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedName;
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid;
}
QString Component::getVersion()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVersion;
}
QString Component::getFilename()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->patchFilePathForUid(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid);
}
QDateTime Component::getReleaseDateTime()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion)
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion->time();
    }
    auto vfile = getVersionFile();
    if(vfile)
    {
        return vfile->releaseTime;
    }
    // FIXME: fake
    return QDateTime::currentDateTime();
}

bool Component::isEnabled()
{
    return !canBeDisabled() || !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_disabled;
}

bool Component::canBeDisabled()
{
    return isRemovable() && !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dependencyOnly;
}

bool Component::setEnabled(bool state)
{
    bool intendedDisabled = !state;
    if (!canBeDisabled())
    {
        intendedDisabled = false;
    }
    if(intendedDisabled != hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_disabled)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_disabled = intendedDisabled;
        emit dataChanged();
        return true;
    }
    return false;
}

bool Component::isCustom()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file != nullptr;
}

bool Component::isCustomizable()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion)
    {
        if(getVersionFile())
        {
            return true;
        }
    }
    return false;
}
bool Component::isRemovable()
{
    return !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_important;
}
bool Component::isRevertible()
{
    if (isCustom())
    {
        if(APPLICATION->metadataIndex()->hasUid(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid))
        {
            return true;
        }
    }
    return false;
}
bool Component::isMoveable()
{
    // HACK, FIXME: this was too dumb and wouldn't follow dependency constraints anyway. For now hardcoded to 'true'.
    return true;
}
bool Component::isVersionChangeable()
{
    auto list = getVersionList();
    if(list)
    {
        if(!list->isLoaded())
        {
            list->load(Net::Mode::Online);
        }
        return list->count() != 0;
    }
    return false;
}

void Component::setImportant(bool state)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_important != state)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_important = state;
        emit dataChanged();
    }
}

ProblemSeverity Component::getProblemSeverity() const
{
    auto file = getVersionFile();
    if(file)
    {
        return file->getProblemSeverity();
    }
    return ProblemSeverity::Error;
}

const QList<PatchProblem> Component::getProblems() const
{
    auto file = getVersionFile();
    if(file)
    {
        return file->getProblems();
    }
    return {{ProblemSeverity::Error, QObject::tr("Patch is not loaded yet.")}};
}

void Component::setVersion(const QString& version)
{
    if(version == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version)
    {
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version = version;
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loaded)
    {
        // we are loaded and potentially have state to invalidate
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file)
        {
            // we have a file... explicit version has been changed and there is nothing else to do.
        }
        else
        {
            // we don't have a file, therefore we are loaded with metadata
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVersion = version;
            // see if the meta version is loaded
            auto metaVersion = APPLICATION->metadataIndex()->get(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid, version);
            if(metaVersion->isLoaded())
            {
                // if yes, we can continue with that.
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion = metaVersion;
            }
            else
            {
                // if not, we need loading
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion.reset();
                hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loaded = false;
            }
            updateCachedData();
        }
    }
    else
    {
        // not loaded... assume it will be sorted out later by the update task
    }
    emit dataChanged();
}

bool Component::customize()
{
    if(isCustom())
    {
        return false;
    }

    auto filename = getFilename();
    if(!FS::ensureFilePathExists(filename))
    {
        return false;
    }
    // FIXME: get rid of this try-catch.
    try
    {
        QSaveFile jsonFile(filename);
        if(!jsonFile.open(QIODevice::WriteOnly))
        {
            return false;
        }
        auto vfile = getVersionFile();
        if(!vfile)
        {
            return false;
        }
        auto document = OneSixVersionFormat::versionFileToJson(vfile);
        jsonFile.write(document.toJson());
        if(!jsonFile.commit())
        {
            return false;
        }
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file = vfile;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion.reset();
        emit dataChanged();
    }
    catch (const Exception &error)
    {
        qWarning() << "Version could not be loaded:" << error.cause();
    }
    return true;
}

bool Component::revert()
{
    if(!isCustom())
    {
        // already not custom
        return true;
    }
    auto filename = getFilename();
    bool result = true;
    // just kill the file and reload
    if(QFile::exists(filename))
    {
        result = QFile::remove(filename);
    }
    if(result)
    {
        // file gone...
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file.reset();

        // check local cache for metadata...
        auto version = APPLICATION->metadataIndex()->get(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_uid, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version);
        if(version->isLoaded())
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion = version;
        }
        else
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_metaVersion.reset();
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_loaded = false;
        }
        emit dataChanged();
    }
    return result;
}

/**
 * deep inspecting compare for requirement sets
 * By default, only uids are compared for set operations.
 * This compares all fields of the Require structs in the sets.
 */
static bool deepCompare(const std::set<Meta::Require> & a, const std::set<Meta::Require> & b)
{
    // NOTE: this needs to be rewritten if the type of Meta::RequireSet changes
    if(a.size() != b.size())
    {
        return false;
    }
    for(const auto & reqA :a)
    {
        const auto &iter2 = b.find(reqA);
        if(iter2 == b.cend())
        {
            return false;
        }
        const auto & reqB = *iter2;
        if(!reqA.deepEquals(reqB))
        {
            return false;
        }
    }
    return true;
}

void Component::updateCachedData()
{
    auto file = getVersionFile();
    if(file)
    {
        bool changed = false;
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedName != file->name)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedName = file->name;
            changed = true;
        }
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVersion != file->version)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVersion = file->version;
            changed = true;
        }
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVolatile != file->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_volatile)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedVolatile = file->hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_volatile;
            changed = true;
        }
        if(!deepCompare(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedRequires, file->requires))
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedRequires = file->requires;
            changed = true;
        }
        if(!deepCompare(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedConflicts, file->conflicts))
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedConflicts = file->conflicts;
            changed = true;
        }
        if(changed)
        {
            emit dataChanged();
        }
    }
    else
    {
        // in case we removed all the metadata
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedRequires.clear();
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_cachedConflicts.clear();
        emit dataChanged();
    }
}

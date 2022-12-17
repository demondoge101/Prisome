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
 *      Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 *      Copyright 2021 Petr Mrazek <peterix@gmail.com>
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

#include "ATLPackInstallTask.h"

#include <QtConcurrent>

#include <quazip/quazip.h>

#include "MMCZip.h"
#include "minecraft/OneSixVersionFormat.h"
#include "Version.h"
#include "net/ChecksumValidator.h"
#include "FileSystem.h"
#include "Json.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"
#include "meta/Index.h"
#include "meta/Version.h"
#include "meta/VersionList.h"

#include "BuildConfig.h"
#include "Application.h"

namespace ATLauncher {

static Meta::Version::Ptr getComponentVersion(const QString& uid, const QString& version);

PackInstallTask::PackInstallTask(UserInteractionSupport *support, QString packName, QString version, InstallMode installMode)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_support = support;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_name = packName;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_safe_name = packName.replace(QRegularExpression("[^A-Za-z0-9]"), "");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_name = version;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_install_mode = installMode;
}

bool PackInstallTask::abort()
{
    if(abortable)
    {
        return jobPtr->abort();
    }
    return false;
}

void PackInstallTask::executeTask()
{
    qDebug() << "PackInstallTask::executeTask: " << QThread::currentThreadId();
    auto *netJob = new NetJob("ATLauncher::VersionFetch", APPLICATION->network());
    auto searchUrl = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "packs/%1/versions/%2/Configs.json")
            .arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_safe_name).arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_name);
    netJob->addNetAction(Net::Download::makeByteArray(QUrl(searchUrl), &response));
    jobPtr = netJob;
    jobPtr->start();

    QObject::connect(netJob, &NetJob::succeeded, this, &PackInstallTask::onDownloadSucceeded);
    QObject::connect(netJob, &NetJob::failed, this, &PackInstallTask::onDownloadFailed);
    QObject::connect(netJob, &NetJob::aborted, this, &PackInstallTask::onDownloadAborted);
}

void PackInstallTask::onDownloadSucceeded()
{
    qDebug() << "PackInstallTask::onDownloadSucceeded: " << QThread::currentThreadId();
    jobPtr.reset();

    QJsonParseError parse_error {};
    QJsonDocument doc = QJsonDocument::fromJson(response, &parse_error);
    if(parse_error.error != QJsonParseError::NoError) {
        qWarning() << "Error while parsing JSON response from ATLauncher at " << parse_error.offset << " reason: " << parse_error.errorString();
        qWarning() << response;
        return;
    }
    auto obj = doc.object();

    ATLauncher::PackVersion version;
    try
    {
        ATLauncher::loadVersion(version, obj);
    }
    catch (const JSONValidationError &e)
    {
        emitFailed(tr("Could not understand pack manifest:\n") + e.cause());
        return;
    }
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version = version;

    // Derived from the installation mode
    QString message;
    bool resetDirectory;

    switch (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_install_mode) {
    case InstallMode::Reinstall:
    case InstallMode::Update:
        message = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.messages.update;
        resetDirectory = true;
        break;

    case InstallMode::Install:
        message = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.messages.install;
        resetDirectory = false;
        break;

    default:
        emitFailed(tr("Unsupported installation mode"));
        return;
    }

    // Display message if one exists
    if (!message.isEmpty())
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_support->displayMessage(message);

    auto ver = getComponentVersion("net.minecraft", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.minecraft);
    if (!ver) {
        emitFailed(tr("Failed to get local metadata index for '%1' v%2").arg("net.minecraft", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.minecraft));
        return;
    }
    minecraftVersion = ver;

    if (resetDirectory) {
        deleteExistingFiles();
    }

    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.noConfigs) {
        downloadMods();
    }
    else {
        installConfigs();
    }
}

void PackInstallTask::onDownloadFailed(QString reason)
{
    qDebug() << "PackInstallTask::onDownloadFailed: " << QThread::currentThreadId();
    jobPtr.reset();
    emitFailed(reason);
}

void PackInstallTask::onDownloadAborted()
{
    jobPtr.reset();
    emitAborted();
}

void PackInstallTask::deleteExistingFiles()
{
    setStatus(tr("Deleting existing files..."));

    // Setup defaults, as per https://wiki.atlauncher.com/pack-admin/xml/delete
    VersionDeletes deletes;
    deletes.folders.append(VersionDelete{ "root", "mods%s%" });
    deletes.folders.append(VersionDelete{ "root", "configs%s%" });
    deletes.folders.append(VersionDelete{ "root", "bin%s%" });

    // Setup defaults, as per https://wiki.atlauncher.com/pack-admin/xml/keep
    VersionKeeps keeps;
    keeps.files.append(VersionKeep{ "root", "mods%s%PortalGunSounds.pak" });
    keeps.folders.append(VersionKeep{ "root", "mods%s%rei_minimap%s%" });
    keeps.folders.append(VersionKeep{ "root", "mods%s%VoxelMods%s%" });
    keeps.files.append(VersionKeep{ "root", "config%s%NEI.cfg" });
    keeps.files.append(VersionKeep{ "root", "options.txt" });
    keeps.files.append(VersionKeep{ "root", "servers.dat" });

    // Merge with version deletes and keeps
    for (const auto& item : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.deletes.files)
        deletes.files.append(item);
    for (const auto& item : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.deletes.folders)
        deletes.folders.append(item);
    for (const auto& item : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.keeps.files)
        keeps.files.append(item);
    for (const auto& item : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.keeps.folders)
        keeps.folders.append(item);

    auto getPathForBase = [this](const QString& base) {
        auto minecraftPath = FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath, "minecraft");

        if (base == "root") {
            return minecraftPath;
        }
        else if (base == "config") {
            return FS::PathCombine(minecraftPath, "config");
        }
        else {
            qWarning() << "Unrecognised base path" << base;
            return minecraftPath;
        }
    };

    auto convertToSystemPath = [](const QString& path) {
        auto t = path;
        t.replace("%s%", QDir::separator());
        return t;
    };

    auto shouldKeep = [keeps, getPathForBase, convertToSystemPath](const QString& fullPath) {
        for (const auto& item : keeps.files) {
            auto basePath = getPathForBase(item.base);
            auto targetPath = convertToSystemPath(item.target);
            auto path = FS::PathCombine(basePath, targetPath);

            if (fullPath == path) {
                return true;
            }
        }

        for (const auto& item : keeps.folders) {
            auto basePath = getPathForBase(item.base);
            auto targetPath = convertToSystemPath(item.target);
            auto path = FS::PathCombine(basePath, targetPath);

            if (fullPath.startsWith(path)) {
                return true;
            }
        }

        return false;
    };

    // Keep track of files to delete
    QSet<QString> filesToDelete;

    for (const auto& item : deletes.files) {
        auto basePath = getPathForBase(item.base);
        auto targetPath = convertToSystemPath(item.target);
        auto fullPath = FS::PathCombine(basePath, targetPath);

        if (shouldKeep(fullPath))
            continue;

        filesToDelete.insert(fullPath);
    }

    for (const auto& item : deletes.folders) {
        auto basePath = getPathForBase(item.base);
        auto targetPath = convertToSystemPath(item.target);
        auto fullPath = FS::PathCombine(basePath, targetPath);

        QDirIterator it(fullPath, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            auto path = it.next();

            if (shouldKeep(path))
                continue;

            filesToDelete.insert(path);
        }
    }

    // Delete the files
    for (const auto& item : filesToDelete) {
        QFile::remove(item);
    }
}

QString PackInstallTask::getDirForModType(ModType type, QString raw)
{
    switch (type) {
        // Mod types that can either be ignored at this stage, or ignored
        // completely.
        case ModType::Root:
        case ModType::Extract:
        case ModType::Decomp:
        case ModType::TexturePackExtract:
        case ModType::ResourcePackExtract:
        case ModType::MCPC:
            return Q_NULLPTR;
        case ModType::Forge:
            // Forge detection happens later on, if it cannot be detected it will
            // install a jarmod component.
        case ModType::Jar:
            return "jarmods";
        case ModType::Mods:
            return "mods";
        case ModType::Flan:
            return "Flan";
        case ModType::Dependency:
            return FS::PathCombine("mods", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.minecraft);
        case ModType::Ic2Lib:
            return FS::PathCombine("mods", "ic2");
        case ModType::DenLib:
            return FS::PathCombine("mods", "denlib");
        case ModType::Coremods:
            return "coremods";
        case ModType::Plugins:
            return "plugins";
        case ModType::TexturePack:
            return "texturepacks";
        case ModType::ResourcePack:
            return "resourcepacks";
        case ModType::ShaderPack:
            return "shaderpacks";
        case ModType::Millenaire:
            qWarning() << "Unsupported mod type: " + raw;
            return Q_NULLPTR;
        case ModType::Unknown:
            emitFailed(tr("Unknown mod type: %1").arg(raw));
            return Q_NULLPTR;
    }

    return Q_NULLPTR;
}

QString PackInstallTask::getVersionForLoader(QString uid)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.recommended || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.latest || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.choose) {
        auto vlist = APPLICATION->metadataIndex()->get(uid);
        if(!vlist)
        {
            emitFailed(tr("Failed to get local metadata index for %1").arg(uid));
            return Q_NULLPTR;
        }

        if(!vlist->isLoaded()) {
            vlist->load(Net::Mode::Online);
        }

        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.recommended || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.latest) {
            for (int i = 0; i < vlist->versions().size(); i++) {
                auto version = vlist->versions().at(i);
                auto reqs = version->requires();

                // filter by minecraft version, if the loader depends on a certain version.
                // not all mod loaders depend on a given Minecraft version, so we won't do this
                // filtering for those loaders.
                if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.type != "fabric") {
                    auto iter = std::find_if(reqs.begin(), reqs.end(), [](const Meta::Require &req) {
                        return req.uid == "net.minecraft";
                    });
                    if (iter == reqs.end()) continue;
                    if (iter->equalsVersion != hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.minecraft) continue;
                }

                if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.recommended) {
                    // first recommended build we find, we use.
                    if (!version->isRecommended()) continue;
                }

                return version->descriptor();
            }

            emitFailed(tr("Failed to find version for %1 loader").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.type));
            return Q_NULLPTR;
        }
        else if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.choose) {
            // Fabric Loader doesn't depend on a given Minecraft version.
            if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.type == "fabric") {
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_support->chooseVersion(vlist, Q_NULLPTR);
            }

            return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_support->chooseVersion(vlist, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.minecraft);
        }
    }

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.version == Q_NULLPTR || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.version.isEmpty()) {
        emitFailed(tr("No loader version set for modpack!"));
        return Q_NULLPTR;
    }

    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.version;
}

QString PackInstallTask::detectLibrary(VersionLibrary library)
{
    // Try to detect what the library is
    if (!library.server.isEmpty() && library.server.split("/").length() >= 3) {
        auto lastSlash = library.server.lastIndexOf("/");
        auto locationAndVersion = library.server.mid(0, lastSlash);
        auto fileName = library.server.mid(lastSlash + 1);

        lastSlash = locationAndVersion.lastIndexOf("/");
        auto location = locationAndVersion.mid(0, lastSlash);
        auto version = locationAndVersion.mid(lastSlash + 1);

        lastSlash = location.lastIndexOf("/");
        auto group = location.mid(0, lastSlash).replace("/", ".");
        auto artefact = location.mid(lastSlash + 1);

        return group + ":" + artefact + ":" + version;
    }

    if(library.file.contains("-")) {
        auto lastSlash = library.file.lastIndexOf("-");
        auto name = library.file.mid(0, lastSlash);
        auto version = library.file.mid(lastSlash + 1).remove(".jar");

        if(name == QString("guava")) {
            return "com.google.guava:guava:" + version;
        }
        else if(name == QString("commons-lang3")) {
            return "org.apache.commons:commons-lang3:" + version;
        }
    }

    return "org.multimc.atlauncher:" + library.md5 + ":1";
}

bool PackInstallTask::createLibrariesComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.libraries.isEmpty()) {
        return true;
    }

    QList<GradleSpecifier> exempt;
    for(const auto & componentUid : componentsToInstall.keys()) {
        auto componentVersion = componentsToInstall.value(componentUid);

        for(const auto & library : componentVersion->data()->libraries) {
            GradleSpecifier lib(library->rawName());
            exempt.append(lib);
        }
    }

    {
        for(const auto & library : minecraftVersion->data()->libraries) {
            GradleSpecifier lib(library->rawName());
            exempt.append(lib);
        }
    }

    auto uuid = QUuid::createUuid();
    auto id = uuid.toString().remove('{').remove('}');
    auto target_id = "org.multimc.atlauncher." + id;

    auto patchDir = FS::PathCombine(instanceRoot, "patches");
    if(!FS::ensureFolderPathExists(patchDir))
    {
        return false;
    }
    auto patchFileName = FS::PathCombine(patchDir, target_id + ".json");

    auto f = std::make_shared<VersionFile>();
    f->name = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_name + " " + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_name + " (libraries)";

    const static QMap<QString, QString> liteLoaderMap = {
            { "61179803bcd5fb7790789b790908663d", "1.12-SNAPSHOT" },
            { "1420785ecbfed5aff4a586c5c9dd97eb", "1.12.2-SNAPSHOT" },
            { "073f68e2fcb518b91fd0d99462441714", "1.6.2_03" },
            { "10a15b52fc59b1bfb9c05b56de1097d6", "1.6.2_02" },
            { "b52f90f08303edd3d4c374e268a5acf1", "1.6.2_04" },
            { "ea747e24e03e24b7cad5bc8a246e0319", "1.6.2_01" },
            { "55785ccc82c07ff0ba038fe24be63ea2", "1.7.10_01" },
            { "63ada46e033d0cb6782bada09ad5ca4e", "1.7.10_04" },
            { "7983e4b28217c9ae8569074388409c86", "1.7.10_03" },
            { "c09882458d74fe0697c7681b8993097e", "1.7.10_02" },
            { "db7235aefd407ac1fde09a7baba50839", "1.7.10_00" },
            { "6e9028816027f53957bd8fcdfabae064", "1.8" },
            { "5e732dc446f9fe2abe5f9decaec40cde", "1.10-SNAPSHOT" },
            { "3a98b5ed95810bf164e71c1a53be568d", "1.11.2-SNAPSHOT" },
            { "ba8e6285966d7d988a96496f48cbddaa", "1.8.9-SNAPSHOT" },
            { "8524af3ac3325a82444cc75ae6e9112f", "1.11-SNAPSHOT" },
            { "53639d52340479ccf206a04f5e16606f", "1.5.2_01" },
            { "1fcdcf66ce0a0806b7ad8686afdce3f7", "1.6.4_00" },
            { "531c116f71ae2b11033f9a11a0f8e668", "1.6.4_01" },
            { "4009eeb99c9068f608d3483a6439af88", "1.7.2_03" },
            { "66f343354b8417abce1a10d557d2c6e9", "1.7.2_04" },
            { "ab554c21f28fbc4ae9b098bcb5f4cceb", "1.7.2_05" },
            { "e1d76a05a3723920e2f80a5e66c45f16", "1.7.2_02" },
            { "00318cb0c787934d523f63cdfe8ddde4", "1.9-SNAPSHOT" },
            { "986fd1ee9525cb0dcab7609401cef754", "1.9.4-SNAPSHOT" },
            { "571ad5e6edd5ff40259570c9be588bb5", "1.9.4" },
            { "1cdd72f7232e45551f16cc8ffd27ccf3", "1.10.2-SNAPSHOT" },
            { "8a7c21f32d77ee08b393dd3921ced8eb", "1.10.2" },
            { "b9bef8abc8dc309069aeba6fbbe58980", "1.12.1-SNAPSHOT" }
    };

    for(const auto & lib : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.libraries) {
        // If the library is LiteLoader, we need to ignore it and handle it separately.
        if (liteLoaderMap.contains(lib.md5)) {
            auto ver = getComponentVersion("com.mumfrey.liteloader", liteLoaderMap.value(lib.md5));
            if (ver) {
                componentsToInstall.insert("com.mumfrey.liteloader", ver);
                continue;
            }
        }

        auto libName = detectLibrary(lib);
        GradleSpecifier libSpecifier(libName);

        bool libExempt = false;
        for(const auto & existingLib : exempt) {
            if(libSpecifier.matchName(existingLib)) {
                // If the pack specifies a newer version of the lib, use that!
                libExempt = Version(libSpecifier.version()) >= Version(existingLib.version());
            }
        }
        if(libExempt) continue;

        auto library = std::make_shared<Library>();
        library->setRawName(libName);

        switch(lib.download) {
            case DownloadType::Server:
                library->setAbsoluteUrl(BuildConfig.ATL_DOWNLOAD_SERVER_URL + lib.url);
                break;
            case DownloadType::Direct:
                library->setAbsoluteUrl(lib.url);
                break;
            case DownloadType::Browser:
            case DownloadType::Unknown:
                emitFailed(tr("Unknown or unsupported download type: %1").arg(lib.download_raw));
                return false;
        }

        f->libraries.append(library);
    }

    if(f->libraries.isEmpty()) {
        return true;
    }

    QFile file(patchFileName);
    if (!file.open(QFile::WriteOnly))
    {
        qCritical() << "Error opening" << file.fileName()
                    << "for reading:" << file.errorString();
        return false;
    }
    file.write(OneSixVersionFormat::versionFileToJson(f).toJson());
    file.close();

    profile->appendComponent(new Component(profile.get(), target_id, f));
    return true;
}

bool PackInstallTask::createPackComponent(QString instanceRoot, std::shared_ptr<PackProfile> profile)
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.mainClass.mainClass.isEmpty() && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.extraArguments.arguments.isEmpty()) {
        return true;
    }

    auto mainClass = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.mainClass.mainClass;
    auto extraArguments = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.extraArguments.arguments;

    auto hasMainClassDepends = !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.mainClass.depends.isEmpty();
    auto hasExtraArgumentsDepends = !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.extraArguments.depends.isEmpty();
    if (hasMainClassDepends || hasExtraArgumentsDepends) {
        QSet<QString> mods;
        for (const auto& item : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.mods) {
            mods.insert(item.name);
        }

        if (hasMainClassDepends && !mods.contains(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.mainClass.depends)) {
            mainClass = "";
        }

        if (hasExtraArgumentsDepends && !mods.contains(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.extraArguments.depends)) {
            extraArguments = "";
        }
    }

    if (mainClass.isEmpty() && extraArguments.isEmpty()) {
        return true;
    }

    auto uuid = QUuid::createUuid();
    auto id = uuid.toString().remove('{').remove('}');
    auto target_id = "org.multimc.atlauncher." + id;

    auto patchDir = FS::PathCombine(instanceRoot, "patches");
    if(!FS::ensureFolderPathExists(patchDir))
    {
        return false;
    }
    auto patchFileName = FS::PathCombine(patchDir, target_id + ".json");

    QStringList mainClasses;
    QStringList tweakers;
    for(const auto & componentUid : componentsToInstall.keys()) {
        auto componentVersion = componentsToInstall.value(componentUid);

        if(componentVersion->data()->mainClass != QString("")) {
            mainClasses.append(componentVersion->data()->mainClass);
        }
        tweakers.append(componentVersion->data()->addTweakers);
    }

    auto f = std::make_shared<VersionFile>();
    f->name = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_name + " " + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_name;
    if (!mainClass.isEmpty() && !mainClasses.contains(mainClass)) {
        f->mainClass = mainClass;
    }

    // Parse out tweakers
    auto args = extraArguments.split(" ");
    QString previous;
    for(auto arg : args) {
        if(arg.startsWith("--tweakClass=") || previous == "--tweakClass") {
            auto tweakClass = arg.remove("--tweakClass=");
            if(tweakers.contains(tweakClass)) continue;

            f->addTweakers.append(tweakClass);
        }
        previous = arg;
    }

    if(f->mainClass == QString() && f->addTweakers.isEmpty()) {
        return true;
    }

    QFile file(patchFileName);
    if (!file.open(QFile::WriteOnly))
    {
        qCritical() << "Error opening" << file.fileName()
                    << "for reading:" << file.errorString();
        return false;
    }
    file.write(OneSixVersionFormat::versionFileToJson(f).toJson());
    file.close();

    profile->appendComponent(new Component(profile.get(), target_id, f));
    return true;
}

void PackInstallTask::installConfigs()
{
    qDebug() << "PackInstallTask::installConfigs: " << QThread::currentThreadId();
    setStatus(tr("Downloading configs..."));
    jobPtr = new NetJob(tr("Config download"), APPLICATION->network());

    auto path = QString("Configs/%1/%2.zip").arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_safe_name).arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_name);
    auto url = QString(BuildConfig.ATL_DOWNLOAD_SERVER_URL + "packs/%1/versions/%2/Configs.zip")
            .arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_safe_name).arg(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_name);
    auto entry = APPLICATION->metacache()->resolveEntry("ATLauncherPacks", path);
    entry->setStale(true);

    auto dl = Net::Download::makeCached(url, entry);
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.configs.sha1.isEmpty()) {
        auto rawSha1 = QByteArray::fromHex(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.configs.sha1.toLatin1());
        dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
    }
    jobPtr->addNetAction(dl);
    archivePath = entry->getFullPath();

    connect(jobPtr.get(), &NetJob::succeeded, this, [&]()
    {
        abortable = false;
        jobPtr.reset();
        extractConfigs();
    });
    connect(jobPtr.get(), &NetJob::failed, [&](QString reason)
    {
        abortable = false;
        jobPtr.reset();
        emitFailed(reason);
    });
    connect(jobPtr.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        abortable = true;
        setProgress(current, total);
    });
    connect(jobPtr.get(), &NetJob::aborted, [&]{
        abortable = false;
        jobPtr.reset();
        emitAborted();
    });

    jobPtr->start();
}

void PackInstallTask::extractConfigs()
{
    qDebug() << "PackInstallTask::extractConfigs: " << QThread::currentThreadId();
    setStatus(tr("Extracting configs..."));

    QDir extractDir(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath);

    QuaZip packZip(archivePath);
    if(!packZip.open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Failed to open pack configs %1!").arg(archivePath));
        return;
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), QOverload<QString, QString>::of(MMCZip::extractDir), archivePath, extractDir.absolutePath() + "/minecraft");
#else
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractDir, archivePath, extractDir.absolutePath() + "/minecraft");
#endif
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, [&]()
    {
        downloadMods();
    });
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, [&]()
    {
        emitAborted();
    });
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFutureWatcher.setFuture(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_extractFuture);
}

void PackInstallTask::downloadMods()
{
    qDebug() << "PackInstallTask::installMods: " << QThread::currentThreadId();

    QVector<ATLauncher::VersionMod> optionalMods;
    for (const auto& mod : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.mods) {
        if (mod.optional) {
            optionalMods.push_back(mod);
        }
    }

    // Select optional mods, if pack contains any
    QVector<QString> selectedMods;
    if (!optionalMods.isEmpty()) {
        setStatus(tr("Selecting optional mods..."));
        auto mods = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_support->chooseOptionalMods(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version, optionalMods);
        if (!mods.has_value()) {
            emitAborted();
            return;
        }
        selectedMods = mods.value();
    }

    setStatus(tr("Downloading mods..."));

    jarmods.clear();
    jobPtr = new NetJob(tr("Mod download"), APPLICATION->network());
    for(const auto& mod : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.mods) {
        // skip non-client mods
        if(!mod.client) continue;

        // skip optional mods that were not selected
        if(mod.optional && !selectedMods.contains(mod.name)) continue;

        QString url;
        switch(mod.download) {
            case DownloadType::Server:
                url = BuildConfig.ATL_DOWNLOAD_SERVER_URL + mod.url;
                break;
            case DownloadType::Browser:
                emitFailed(tr("Unsupported download type: %1").arg(mod.download_raw));
                return;
            case DownloadType::Direct:
                url = mod.url;
                break;
            case DownloadType::Unknown:
                emitFailed(tr("Unknown download type: %1").arg(mod.download_raw));
                return;
        }

        QFileInfo fileName(mod.file);
        auto cacheName = fileName.completeBaseName() + "-" + mod.md5 + "." + fileName.suffix();

        if (mod.type == ModType::Extract || mod.type == ModType::TexturePackExtract || mod.type == ModType::ResourcePackExtract) {
            auto entry = APPLICATION->metacache()->resolveEntry("ATLauncherPacks", cacheName);
            entry->setStale(true);
            modsToExtract.insert(entry->getFullPath(), mod);

            auto dl = Net::Download::makeCached(url, entry);
            if (!mod.md5.isEmpty()) {
                auto rawMd5 = QByteArray::fromHex(mod.md5.toLatin1());
                dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Md5, rawMd5));
            }
            jobPtr->addNetAction(dl);
        }
        else if(mod.type == ModType::Decomp) {
            auto entry = APPLICATION->metacache()->resolveEntry("ATLauncherPacks", cacheName);
            entry->setStale(true);
            modsToDecomp.insert(entry->getFullPath(), mod);

            auto dl = Net::Download::makeCached(url, entry);
            if (!mod.md5.isEmpty()) {
                auto rawMd5 = QByteArray::fromHex(mod.md5.toLatin1());
                dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Md5, rawMd5));
            }
            jobPtr->addNetAction(dl);
        }
        else {
            auto relpath = getDirForModType(mod.type, mod.type_raw);
            if(relpath == Q_NULLPTR) continue;

            auto entry = APPLICATION->metacache()->resolveEntry("ATLauncherPacks", cacheName);
            entry->setStale(true);

            auto dl = Net::Download::makeCached(url, entry);
            if (!mod.md5.isEmpty()) {
                auto rawMd5 = QByteArray::fromHex(mod.md5.toLatin1());
                dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Md5, rawMd5));
            }
            jobPtr->addNetAction(dl);

            auto path = FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath, "minecraft", relpath, mod.file);

            if(mod.type == ModType::Forge) {
                auto ver = getComponentVersion("net.minecraftforge", mod.version);
                if (ver) {
                    componentsToInstall.insert("net.minecraftforge", ver);
                    continue;
                }

                qDebug() << "Jarmod: " + path;
                jarmods.push_back(path);
            }

            if(mod.type == ModType::Jar) {
                qDebug() << "Jarmod: " + path;
                jarmods.push_back(path);
            }

            // Download after Forge handling, to avoid downloading Forge twice.
            qDebug() << "Will download" << url << "to" << path;
            modsToCopy[entry->getFullPath()] = path;
        }
    }

    connect(jobPtr.get(), &NetJob::succeeded, this, &PackInstallTask::onModsDownloaded);
    connect(jobPtr.get(), &NetJob::failed, [&](QString reason)
    {
        abortable = false;
        jobPtr.reset();
        emitFailed(reason);
    });
    connect(jobPtr.get(), &NetJob::progress, [&](qint64 current, qint64 total)
    {
        abortable = true;
        setProgress(current, total);
    });
    connect(jobPtr.get(), &NetJob::aborted, [&]
    {
        abortable = false;
        jobPtr.reset();
        emitAborted();
    });

    jobPtr->start();
}

void PackInstallTask::onModsDownloaded() {
    abortable = false;

    qDebug() << "PackInstallTask::onModsDownloaded: " << QThread::currentThreadId();
    jobPtr.reset();

    if(!modsToExtract.empty() || !modsToDecomp.empty() || !modsToCopy.empty()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modExtractFuture = QtConcurrent::run(QThreadPool::globalInstance(), &PackInstallTask::extractMods, this, modsToExtract, modsToDecomp, modsToCopy);
#else
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modExtractFuture = QtConcurrent::run(QThreadPool::globalInstance(), this, &PackInstallTask::extractMods, modsToExtract, modsToDecomp, modsToCopy);
#endif
        connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modExtractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &PackInstallTask::onModsExtracted);
        connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modExtractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, [&]()
        {
            emitAborted();
        });
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modExtractFutureWatcher.setFuture(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modExtractFuture);
    }
    else {
        install();
    }
}

void PackInstallTask::onModsExtracted() {
    qDebug() << "PackInstallTask::onModsExtracted: " << QThread::currentThreadId();
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modExtractFuture.result()) {
        install();
    }
    else {
        emitFailed(tr("Failed to extract mods..."));
    }
}

bool PackInstallTask::extractMods(
    const QMap<QString, VersionMod> &toExtract,
    const QMap<QString, VersionMod> &toDecomp,
    const QMap<QString, QString> &toCopy
) {
    qDebug() << "PackInstallTask::extractMods: " << QThread::currentThreadId();

    setStatus(tr("Extracting mods..."));
    for (auto iter = toExtract.begin(); iter != toExtract.end(); iter++) {
        auto &modPath = iter.key();
        auto &mod = iter.value();

        QString extractToDir;
        if(mod.type == ModType::Extract) {
            extractToDir = getDirForModType(mod.extractTo, mod.extractTo_raw);
        }
        else if(mod.type == ModType::TexturePackExtract) {
            extractToDir = FS::PathCombine("texturepacks", "extracted");
        }
        else if(mod.type == ModType::ResourcePackExtract) {
            extractToDir = FS::PathCombine("resourcepacks", "extracted");
        }

        QDir extractDir(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath);
        auto extractToPath = FS::PathCombine(extractDir.absolutePath(), "minecraft", extractToDir);

        QString folderToExtract = "";
        if(mod.type == ModType::Extract) {
            folderToExtract = mod.extractFolder;
            folderToExtract.remove(QRegularExpression("^/"));
        }

        qDebug() << "Extracting " + mod.file + " to " + extractToDir;
        if(!MMCZip::extractDir(modPath, folderToExtract, extractToPath)) {
            // assume error
            return false;
        }
    }

    for (auto iter = toDecomp.begin(); iter != toDecomp.end(); iter++) {
        auto &modPath = iter.key();
        auto &mod = iter.value();
        auto extractToDir = getDirForModType(mod.decompType, mod.decompType_raw);

        QDir extractDir(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath);
        auto extractToPath = FS::PathCombine(extractDir.absolutePath(), "minecraft", extractToDir, mod.decompFile);

        qDebug() << "Extracting " + mod.decompFile + " to " + extractToDir;
        if(!MMCZip::extractFile(modPath, mod.decompFile, extractToPath)) {
            qWarning() << "Failed to extract" << mod.decompFile;
            return false;
        }
    }

    for (auto iter = toCopy.begin(); iter != toCopy.end(); iter++) {
        auto &from = iter.key();
        auto &to = iter.value();

        // If the file already exists, assume the mod is the correct copy - and remove
        // the copy from the Configs.zip
        QFileInfo fileInfo(to);
        if (fileInfo.exists()) {
            if (!QFile::remove(to)) {
                qWarning() << "Failed to delete" << to;
                return false;
            }
        }

        FS::copy fileCopyOperation(from, to);
        if(!fileCopyOperation()) {
            qWarning() << "Failed to copy" << from << "to" << to;
            return false;
        }
    }
    return true;
}

void PackInstallTask::install()
{
    qDebug() << "PackInstallTask::install: " << QThread::currentThreadId();
    setStatus(tr("Installing modpack"));

    auto instanceConfigPath = FS::PathCombine(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
    instanceSettings->suspendSave();

    MinecraftInstance instance(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_globalSettings, instanceSettings, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_stagingPath);
    auto components = instance.getPackProfile();
    components->buildingFromScratch();

    // Use a component to add libraries BEFORE Minecraft
    if(!createLibrariesComponent(instance.instanceRoot(), components)) {
        emitFailed(tr("Failed to create libraries component"));
        return;
    }

    // Minecraft
    components->setComponentVersion("net.minecraft", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.minecraft, true);

    // Loader
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.type == QString("forge"))
    {
        auto version = getVersionForLoader("net.minecraftforge");
        if(version == Q_NULLPTR) return;

        components->setComponentVersion("net.minecraftforge", version);
    }
    else if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.type == QString("fabric"))
    {
        auto version = getVersionForLoader("net.fabricmc.fabric-loader");
        if(version == Q_NULLPTR) return;

        components->setComponentVersion("net.fabricmc.fabric-loader", version);
    }
    else if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.type != QString())
    {
        emitFailed(tr("Unknown loader type: ") + hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version.loader.type);
        return;
    }

    for(const auto & componentUid : componentsToInstall.keys()) {
        auto version = componentsToInstall.value(componentUid);
        components->setComponentVersion(componentUid, version->version());
    }

    components->installJarMods(jarmods);

    // Use a component to fill in the rest of the data
    // todo: use more detection
    if(!createPackComponent(instance.instanceRoot(), components)) {
        emitFailed(tr("Failed to create pack component"));
        return;
    }

    components->saveNow();

    instance.setName(name());
    instance.setIconKey(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instIcon);
    instance.setManagedPack("atlauncher", hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_safe_name, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_name, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_name, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_version_name);
    instanceSettings->resumeSave();

    jarmods.clear();
    emitSucceeded();
}

static Meta::Version::Ptr getComponentVersion(const QString& uid, const QString& version)
{
    auto vlist = APPLICATION->metadataIndex()->get(uid);
    if (!vlist)
        return {};

    if (!vlist->isLoaded())
        vlist->load(Net::Mode::Online);

    auto ver = vlist->getVersion(version);
    if (!ver)
        return {};

    if (!ver->isLoaded())
        ver->load(Net::Mode::Online);

    return ver;
}

}

// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "FlamePackExportTask.h"
#include <QJsonArray>
#include <QJsonObject>

#include <QCryptographicHash>
#include <QFileInfo>
#include <QMessageBox>
#include <QtConcurrentRun>
#include <algorithm>
#include <memory>
#include "Json.h"
#include "MMCZip.h"
#include "minecraft/PackProfile.h"
#include "minecraft/mod/ModFolderModel.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameModIndex.h"
#include "modplatform/helpers/HashUtils.h"
#include "tasks/Task.h"

const QString FlamePackExportTask::TEMPLATE = "<li><a href={url}>{name}{authors}</a></li>\n";
const QStringList FlamePackExportTask::FILE_EXTENSIONS({ "jar", "zip" });

FlamePackExportTask::FlamePackExportTask(const QString& name,
                                         const QString& version,
                                         const QString& author,
                                         InstancePtr instance,
                                         const QString& output,
                                         MMCZip::FilterFunction filter)
    : name(name)
    , version(version)
    , author(author)
    , instance(instance)
    , mcInstance(dynamic_cast<MinecraftInstance*>(instance.get()))
    , gameRoot(instance->gameRoot())
    , output(output)
    , filter(filter)
{}

void FlamePackExportTask::executeTask()
{
    setStatus(tr("Searching for files..."));
    setProgress(0, 5);
    collectFiles();
}

bool FlamePackExportTask::abort()
{
    if (task != nullptr) {
        task->abort();
        task = nullptr;
        emitAborted();
        return true;
    }

    if (buildZipFuture.isRunning()) {
        buildZipFuture.cancel();
        // NOTE: Here we don't do `emitAborted()` because it will be done when `buildZipFuture` actually cancels, which may not occur
        // immediately.
        return true;
    }

    return false;
}

void FlamePackExportTask::collectFiles()
{
    setAbortable(false);
    QCoreApplication::processEvents();

    files.clear();
    if (!MMCZip::collectFileListRecursively(instance->gameRoot(), nullptr, &files, filter)) {
        emitFailed(tr("Could not search for files"));
        return;
    }

    pendingHashes.clear();
    resolvedFiles.clear();

    if (mcInstance != nullptr) {
        mcInstance->loaderModList()->update();
        connect(mcInstance->loaderModList().get(), &ModFolderModel::updateFinished, this, &FlamePackExportTask::collectHashes);
    } else
        collectHashes();
}

void FlamePackExportTask::collectHashes()
{
    setAbortable(true);
    setStatus(tr("Find file hashes..."));
    setProgress(1, 5);
    auto allMods = mcInstance->loaderModList()->allMods();
    ConcurrentTask::Ptr hashing_task(new ConcurrentTask(this, "MakeHashesTask", 10));
    task.reset(hashing_task);
    for (const QFileInfo& file : files) {
        const QString relative = gameRoot.relativeFilePath(file.absoluteFilePath());
        // require sensible file types
        if (!std::any_of(FILE_EXTENSIONS.begin(), FILE_EXTENSIONS.end(), [&relative](const QString& extension) {
                return relative.endsWith('.' + extension) || relative.endsWith('.' + extension + ".disabled");
            }))
            continue;

        if (relative.startsWith("resourcepacks/") &&
            (relative.endsWith(".zip") || relative.endsWith(".zip.disabled"))) {  // is resourcepack
            auto hash_task = Hashing::createFlameHasher(file.absoluteFilePath());
            connect(hash_task.get(), &Hashing::Hasher::resultsReady, [this, relative, file](QString hash) {
                if (m_state == Task::State::Running) {
                    pendingHashes.insert(hash, { relative, file.absoluteFilePath(), relative.endsWith(".zip") });
                }
            });
            connect(hash_task.get(), &Task::failed, this, &FlamePackExportTask::emitFailed);
            hashing_task->addTask(hash_task);
            continue;
        }

        if (auto modIter = std::find_if(allMods.begin(), allMods.end(), [&file](Mod* mod) { return mod->fileinfo() == file; });
            modIter != allMods.end()) {
            const Mod* mod = *modIter;
            if (!mod || mod->type() == ResourceType::FOLDER) {
                continue;
            }
            if (mod->metadata() && mod->metadata()->provider == ModPlatform::ResourceProvider::FLAME) {
                resolvedFiles.insert(mod->fileinfo().absoluteFilePath(),
                                     { mod->metadata()->project_id.toInt(), mod->metadata()->file_id.toInt(), mod->enabled(), true,
                                       mod->metadata()->name, mod->metadata()->slug, mod->authors().join(", ") });
                continue;
            }

            auto hash_task = Hashing::createFlameHasher(mod->fileinfo().absoluteFilePath());
            connect(hash_task.get(), &Hashing::Hasher::resultsReady, [this, mod](QString hash) {
                if (m_state == Task::State::Running) {
                    pendingHashes.insert(hash, { mod->name(), mod->fileinfo().absoluteFilePath(), mod->enabled(), true });
                }
            });
            connect(hash_task.get(), &Task::failed, this, &FlamePackExportTask::emitFailed);
            hashing_task->addTask(hash_task);
        }
    }
    auto step_progress = std::make_shared<TaskStepProgress>();
    connect(hashing_task.get(), &Task::finished, this, [this, step_progress] {
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
    });

    connect(hashing_task.get(), &Task::succeeded, this, &FlamePackExportTask::makeApiRequest);
    connect(hashing_task.get(), &Task::failed, this, [this, step_progress](QString reason) {
        step_progress->state = TaskStepState::Failed;
        stepProgress(*step_progress);
        emitFailed(reason);
    });
    connect(hashing_task.get(), &Task::stepProgress, this, &FlamePackExportTask::propogateStepProgress);

    connect(hashing_task.get(), &Task::progress, this, [this, step_progress](qint64 current, qint64 total) {
        step_progress->update(current, total);
        stepProgress(*step_progress);
    });
    connect(hashing_task.get(), &Task::status, this, [this, step_progress](QString status) {
        step_progress->status = status;
        stepProgress(*step_progress);
    });
    hashing_task->start();
}

void FlamePackExportTask::makeApiRequest()
{
    if (pendingHashes.isEmpty()) {
        buildZip();
        return;
    }

    setStatus(tr("Find versions for hashes..."));
    setProgress(2, 5);
    auto response = std::make_shared<QByteArray>();

    QList<uint> fingerprints;
    for (auto& murmur : pendingHashes.keys()) {
        fingerprints.push_back(murmur.toUInt());
    }

    task.reset(api.matchFingerprints(fingerprints, response));

    connect(task.get(), &Task::succeeded, this, [this, response] {
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from CurseForge::CurrentVersions at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;

            failed(parse_error.errorString());
            return;
        }

        try {
            auto doc_obj = Json::requireObject(doc);
            auto data_obj = Json::requireObject(doc_obj, "data");
            auto data_arr = Json::requireArray(data_obj, "exactMatches");

            if (data_arr.isEmpty()) {
                qWarning() << "No matches found for fingerprint search!";

                return;
            }
            for (auto match : data_arr) {
                auto match_obj = Json::ensureObject(match, {});
                auto file_obj = Json::ensureObject(match_obj, "file", {});

                if (match_obj.isEmpty() || file_obj.isEmpty()) {
                    qWarning() << "Fingerprint match is empty!";

                    return;
                }

                auto fingerprint = QString::number(Json::ensureVariant(file_obj, "fileFingerprint").toUInt());
                auto mod = pendingHashes.find(fingerprint);
                if (mod == pendingHashes.end()) {
                    qWarning() << "Invalid fingerprint from the API response.";
                    continue;
                }

                setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(mod->name));
                if (Json::ensureBoolean(file_obj, "isAvailable", false, "isAvailable"))
                    resolvedFiles.insert(mod->path, { Json::requireInteger(file_obj, "modId"), Json::requireInteger(file_obj, "id"),
                                                      mod->enabled, mod->isMod });
            }

        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
        pendingHashes.clear();
    });
    connect(task.get(), &Task::finished, this, &FlamePackExportTask::getProjectsInfo);
    connect(task.get(), &NetJob::failed, this, &FlamePackExportTask::emitFailed);
    task->start();
}

void FlamePackExportTask::getProjectsInfo()
{
    setStatus(tr("Find project info from CurseForge..."));
    setProgress(3, 5);
    QList<QString> addonIds;
    for (auto resolved : resolvedFiles) {
        if (resolved.slug.isEmpty()) {
            addonIds << QString::number(resolved.addonId);
        }
    }

    auto response = std::make_shared<QByteArray>();
    Task::Ptr proj_task;

    if (addonIds.isEmpty()) {
        buildZip();
        return;
    } else if (addonIds.size() == 1) {
        proj_task = api.getProject(*addonIds.begin(), response);
    } else {
        proj_task = api.getProjects(addonIds, response);
    }

    connect(proj_task.get(), &Task::succeeded, this, [this, response, addonIds] {
        QJsonParseError parse_error{};
        auto doc = QJsonDocument::fromJson(*response, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response from CurseForge projects task at " << parse_error.offset
                       << " reason: " << parse_error.errorString();
            qWarning() << *response;
            failed(parse_error.errorString());
            return;
        }

        try {
            QJsonArray entries;
            if (addonIds.size() == 1)
                entries = { Json::requireObject(Json::requireObject(doc), "data") };
            else
                entries = Json::requireArray(Json::requireObject(doc), "data");

            for (auto entry : entries) {
                auto entry_obj = Json::requireObject(entry);

                try {
                    setStatus(tr("Parsing API response from CurseForge for '%1'...").arg(Json::requireString(entry_obj, "name")));

                    ModPlatform::IndexedPack pack;
                    FlameMod::loadIndexedPack(pack, entry_obj);
                    for (auto key : resolvedFiles.keys()) {
                        auto val = resolvedFiles.value(key);
                        if (val.addonId == pack.addonId) {
                            val.name = pack.name;
                            val.slug = pack.slug;
                            QStringList authors;
                            for (auto author : pack.authors)
                                authors << author.name;

                            val.authors = authors.join(", ");
                            resolvedFiles[key] = val;
                        }
                    }

                } catch (Json::JsonException& e) {
                    qDebug() << e.cause();
                    qDebug() << entries;
                }
            }
        } catch (Json::JsonException& e) {
            qDebug() << e.cause();
            qDebug() << doc;
        }
        buildZip();
    });
    task.reset(proj_task);
    task->start();
}

void FlamePackExportTask::buildZip()
{
    setStatus(tr("Adding files..."));
    setProgress(4, 5);

    buildZipFuture = QtConcurrent::run(QThreadPool::globalInstance(), [this]() {
        QuaZip zip(output);
        if (!zip.open(QuaZip::mdCreate)) {
            QFile::remove(output);
            return BuildZipResult(tr("Could not create file"));
        }

        if (buildZipFuture.isCanceled())
            return BuildZipResult();

        QuaZipFile indexFile(&zip);
        if (!indexFile.open(QIODevice::WriteOnly, QuaZipNewInfo("manifest.json"))) {
            QFile::remove(output);
            return BuildZipResult(tr("Could not create index"));
        }
        indexFile.write(generateIndex());

        QuaZipFile modlist(&zip);
        if (!modlist.open(QIODevice::WriteOnly, QuaZipNewInfo("modlist.html"))) {
            QFile::remove(output);
            return BuildZipResult(tr("Could not create index"));
        }
        QString content = "";
        for (auto mod : resolvedFiles) {
            if (mod.isMod) {
                content += QString(TEMPLATE)
                               .replace("{name}", mod.name)
                               .replace("{url}", ModPlatform::getMetaURL(ModPlatform::ResourceProvider::FLAME, mod.addonId))
                               .replace("{authors}", !mod.authors.isEmpty() ? QString(" (by %1)").arg(mod.authors) : "");
            }
        }
        content = "<ul>" + content + "</ul>";
        modlist.write(content.toUtf8());

        auto step_progress = std::make_shared<TaskStepProgress>();

        size_t progress = 0;
        for (const QFileInfo& file : files) {
            if (buildZipFuture.isCanceled()) {
                QFile::remove(output);
                step_progress->state = TaskStepState::Failed;
                stepProgress(*step_progress);
                return BuildZipResult();
            }
            step_progress->update(progress, files.length());
            stepProgress(*step_progress);

            const QString relative = gameRoot.relativeFilePath(file.absoluteFilePath());
            if (!resolvedFiles.contains(file.absoluteFilePath()) &&
                !JlCompress::compressFile(&zip, file.absoluteFilePath(), "overrides/" + relative)) {
                QFile::remove(output);
                return BuildZipResult(tr("Could not read and compress %1").arg(relative));
            }
            progress++;
        }

        zip.close();

        if (zip.getZipError() != 0) {
            QFile::remove(output);
            step_progress->state = TaskStepState::Failed;
            stepProgress(*step_progress);
            return BuildZipResult(tr("A zip error occurred"));
        }
        step_progress->state = TaskStepState::Succeeded;
        stepProgress(*step_progress);
        return BuildZipResult();
    });
    connect(&buildZipWatcher, &QFutureWatcher<BuildZipResult>::finished, this, &FlamePackExportTask::finish);
    buildZipWatcher.setFuture(buildZipFuture);
}

void FlamePackExportTask::finish()
{
    if (buildZipFuture.isCanceled())
        emitAborted();
    else {
        const BuildZipResult result = buildZipFuture.result();
        if (result.has_value())
            emitFailed(result.value());
        else
            emitSucceeded();
    }
}

QByteArray FlamePackExportTask::generateIndex()
{
    QJsonObject obj;
    obj["manifestType"] = "minecraftModpack";
    obj["manifestVersion"] = 1;
    obj["name"] = name;
    obj["version"] = version;
    obj["author"] = author;
    obj["overrides"] = "overrides";
    if (mcInstance) {
        QJsonObject version;
        auto profile = mcInstance->getPackProfile();
        // collect all supported components
        const ComponentPtr minecraft = profile->getComponent("net.minecraft");
        const ComponentPtr quilt = profile->getComponent("org.quiltmc.quilt-loader");
        const ComponentPtr fabric = profile->getComponent("net.fabricmc.fabric-loader");
        const ComponentPtr forge = profile->getComponent("net.minecraftforge");

        // convert all available components to mrpack dependencies
        if (minecraft != nullptr)
            version["version"] = minecraft->m_version;
        QString id;
        if (quilt != nullptr)
            id = "quilt-" + quilt->getVersion();
        else if (fabric != nullptr)
            id = "fabric-" + fabric->getVersion();
        else if (forge != nullptr)
            id = "forge-" + forge->getVersion();
        version["modLoaders"] = QJsonArray();
        if (!id.isEmpty()) {
            QJsonObject loader;
            loader["id"] = id;
            loader["primary"] = true;
            version["modLoaders"] = QJsonArray({ loader });
        }
        obj["minecraft"] = version;
    }

    QJsonArray files;
    for (auto mod : resolvedFiles) {
        QJsonObject file;
        file["projectID"] = mod.addonId;
        file["fileID"] = mod.version;
        file["required"] = mod.enabled;
        files << file;
    }
    obj["files"] = files;

    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}
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

#include "LocalResourcePackParseTask.h"

#include "FileSystem.h"
#include "Json.h"

#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

#include <QCryptographicHash>

namespace ResourcePackUtils {

bool process(ResourcePack& pack, ProcessingLevel level)
{
    switch (pack.type()) {
        case ResourceType::FOLDER:
            ResourcePackUtils::processFolder(pack, level);
            return true;
        case ResourceType::ZIPFILE:
            ResourcePackUtils::processZIP(pack, level);
            return true;
        default:
            qWarning() << "Invalid type for resource pack parse task!";
            return false;
    }
}

void processFolder(ResourcePack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::FOLDER);

    QFileInfo mcmeta_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.mcmeta"));
    if (mcmeta_file_info.isFile()) {
        QFile mcmeta_file(mcmeta_file_info.filePath());
        if (!mcmeta_file.open(QIODevice::ReadOnly))
            return;

        auto data = mcmeta_file.readAll();

        ResourcePackUtils::processMCMeta(pack, std::move(data));

        mcmeta_file.close();
    }

    if (level == ProcessingLevel::BasicInfoOnly)
        return;

    QFileInfo image_file_info(FS::PathCombine(pack.fileinfo().filePath(), "pack.png"));
    if (image_file_info.isFile()) {
        QFile mcmeta_file(image_file_info.filePath());
        if (!mcmeta_file.open(QIODevice::ReadOnly))
            return;

        auto data = mcmeta_file.readAll();

        ResourcePackUtils::processPackPNG(pack, std::move(data));

        mcmeta_file.close();
    }
}

void processZIP(ResourcePack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::ZIPFILE);

    QuaZip zip(pack.fileinfo().filePath());
    if (!zip.open(QuaZip::mdUnzip))
        return;

    QuaZipFile file(&zip);

    if (zip.setCurrentFile("pack.mcmeta")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open file in zip.";
            zip.close();
            return;
        }

        auto data = file.readAll();

        ResourcePackUtils::processMCMeta(pack, std::move(data));

        file.close();
    }

    if (level == ProcessingLevel::BasicInfoOnly) {
        zip.close();
        return;
    }

    if (zip.setCurrentFile("pack.png")) {
        if (!file.open(QIODevice::ReadOnly)) {
            qCritical() << "Failed to open file in zip.";
            zip.close();
            return;
        }

        auto data = file.readAll();

        ResourcePackUtils::processPackPNG(pack, std::move(data));

        file.close();
    }

    zip.close();
}

// https://minecraft.fandom.com/wiki/Tutorials/Creating_a_resource_pack#Formatting_pack.mcmeta
void processMCMeta(ResourcePack& pack, QByteArray&& raw_data)
{
    try {
        auto json_doc = QJsonDocument::fromJson(raw_data);
        auto pack_obj = Json::requireObject(json_doc.object(), "pack", {});

        pack.setPackFormat(Json::ensureInteger(pack_obj, "pack_format", 0));
        pack.setDescription(Json::ensureString(pack_obj, "description", ""));
    } catch (Json::JsonException& e) {
        qWarning() << "JsonException: " << e.what() << e.cause();
    }
}

void processPackPNG(ResourcePack& pack, QByteArray&& raw_data)
{
    auto img = QImage::fromData(raw_data);
    if (!img.isNull()) {
        pack.setImage(img);
    } else {
        qWarning() << "Failed to parse pack.png.";
    }
}

bool validate(QFileInfo file)
{
    ResourcePack rp{ file };
    return ResourcePackUtils::process(rp, ProcessingLevel::BasicInfoOnly) && rp.valid();
}

}  // namespace ResourcePackUtils

LocalResourcePackParseTask::LocalResourcePackParseTask(int token, ResourcePack& rp)
    : Task(nullptr, false), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token(token), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_resource_pack(rp)
{}

bool LocalResourcePackParseTask::abort()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted = true;
    return true;
}

void LocalResourcePackParseTask::executeTask()
{
    if (!ResourcePackUtils::process(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_resource_pack))
        return;

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_aborted)
        emitAborted();
    else
        emitSucceeded();
}

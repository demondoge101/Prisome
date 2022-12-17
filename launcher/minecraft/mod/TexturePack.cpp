// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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
 */

#include "TexturePack.h"

#include <QDebug>
#include <QMap>
#include <QRegularExpression>

#include "minecraft/mod/tasks/LocalTexturePackParseTask.h"

void TexturePack::setDescription(QString new_description)
{
    QMutexLocker locker(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data_lock);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_description = new_description;
}

void TexturePack::setImage(QImage new_image)
{
    QMutexLocker locker(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data_lock);

    Q_ASSERT(!new_image.isNull());

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_image_cache_key.key.isValid())
        QPixmapCache::remove(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_image_cache_key.key);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_image_cache_key.key = QPixmapCache::insert(QPixmap::fromImage(new_image));
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_image_cache_key.was_ever_used = true;
}

QPixmap TexturePack::image(QSize size)
{
    QPixmap cached_image;
    if (QPixmapCache::find(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_image_cache_key.key, &cached_image)) {
        if (size.isNull())
            return cached_image;
        return cached_image.scaled(size);
    }

    // No valid image we can get
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_image_cache_key.was_ever_used)
        return {};

    // Imaged got evicted from the cache. Re-process it and retry.
    TexturePackUtils::process(*this);
    return image(size);
}

bool TexturePack::valid() const
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_description != nullptr;
}

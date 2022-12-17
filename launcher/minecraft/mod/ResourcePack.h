#pragma once

#include "Resource.h"

#include <QImage>
#include <QMutex>
#include <QPixmap>
#include <QPixmapCache>

class Version;

/* TODO:
 *
 * Store localized descriptions
 * */

class ResourcePack : public Resource {
    Q_OBJECT
   public:
    using Ptr = shared_qobject_ptr<Resource>;

    ResourcePack(QObject* parent = nullptr) : Resource(parent) {}
    ResourcePack(QFileInfo file_info) : Resource(file_info) {}

    /** Gets the numerical ID of the pack format. */
    [[nodiscard]] int packFormat() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_format; }
    /** Gets, respectively, the lower and upper versions supported by the set pack format. */
    [[nodiscard]] std::pair<Version, Version> compatibleVersions() const;

    /** Gets the description of the resource pack. */
    [[nodiscard]] QString description() const { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_description; }

    /** Gets the image of the resource pack, converted to a QPixmap for drawing, and scaled to size. */
    [[nodiscard]] QPixmap image(QSize size);

    /** Thread-safe. */
    void setPackFormat(int new_format_id);

    /** Thread-safe. */
    void setDescription(QString new_description);

    /** Thread-safe. */
    void setImage(QImage new_image);

    bool valid() const override;

    [[nodiscard]] auto compare(Resource const& other, SortType type) const -> std::pair<int, bool> override;
    [[nodiscard]] bool applyFilter(QRegularExpression filter) const override;

   protected:
    mutable QMutex hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_data_lock;

    /* The 'version' of a resource pack, as defined in the pack.mcmeta file.
     * See https://minecraft.fandom.com/wiki/Tutorials/Creating_a_resource_pack#Formatting_pack.mcmeta
     */
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_format = 0;

    /** The resource pack's description, as defined in the pack.mcmeta file.
     */
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_description;

    /** The resource pack's image file cache key, for access in the QPixmapCache global instance.
     *
     *  The 'was_ever_used' state simply identifies whether the key was never inserted on the cache (true),
     *  so as to tell whether a cache entry is inexistent or if it was just evicted from the cache.
     */
    struct {
        QPixmapCache::Key key;
        bool was_ever_used = false;
    } hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack_image_cache_key;
};

#pragma once

#include "ModIndex.h"
#include "net/NetJob.h"

#include "modplatform/helpers/HashUtils.h"

#include "tasks/ConcurrentTask.h"

class Mod;
class QDir;

class EnsureMetadataTask : public Task {
    Q_OBJECT

   public:
    EnsureMetadataTask(Mod*, QDir, ModPlatform::Provider = ModPlatform::Provider::MODRINTH);
    EnsureMetadataTask(QList<Mod*>&, QDir, ModPlatform::Provider = ModPlatform::Provider::MODRINTH);

    ~EnsureMetadataTask() = default;

    Task::Ptr getHashingTask() { return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hashing_task; }

   public slots:
    bool abort() override;
   protected slots:
    void executeTask() override;

   private:
    // FIXME: Move to their own namespace
    auto modrinthVersionsTask() -> NetJob::Ptr;
    auto modrinthProjectsTask() -> NetJob::Ptr;

    auto flameVersionsTask() -> NetJob::Ptr;
    auto flameProjectsTask() -> NetJob::Ptr;

    // Helpers
    enum class RemoveFromList {
        Yes,
        No
    };
    void emitReady(Mod*, QString key = {}, RemoveFromList = RemoveFromList::Yes);
    void emitFail(Mod*, QString key = {}, RemoveFromList = RemoveFromList::Yes);

    // Hashes and stuff
    auto createNewHash(Mod*) -> Hashing::Hasher::Ptr;
    auto getExistingHash(Mod*) -> QString;

   private slots:
    void modrinthCallback(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Mod*);
    void flameCallback(ModPlatform::IndexedPack& pack, ModPlatform::IndexedVersion& ver, Mod*);

   signals:
    void metadataReady(Mod*);
    void metadataFailed(Mod*);

   private:
    QHash<QString, Mod*> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mods;
    QDir hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_index_dir;
    ModPlatform::Provider hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_provider;

    QHash<QString, ModPlatform::IndexedVersion> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_temp_versions;
    ConcurrentTask* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hashing_task;
    NetJob* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_current_task;
};

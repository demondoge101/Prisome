#pragma once

#include "minecraft/mod/Mod.h"
#include "minecraft/mod/tasks/GetModDependenciesTask.h"
#include "modplatform/ModIndex.h"
#include "modplatform/ResourceAPI.h"
#include "tasks/Task.h"

class ResourceDownloadTask;
class ModFolderModel;

class CheckUpdateTask : public Task {
    Q_OBJECT

   public:
    CheckUpdateTask(QList<Resource*>& resources,
                    std::list<Version>& mcVersions,
                    std::optional<ModPlatform::ModLoaderTypes> loaders,
                    std::shared_ptr<ResourceFolderModel> resource_model)
        : Task(nullptr), m_resources(resources), m_game_versions(mcVersions), m_loaders(loaders), m_resource_model(resource_model){};

    struct Update {
        QString name;
        QString old_hash;
        QString old_version;
        QString new_version;
        std::optional<ModPlatform::IndexedVersionType> new_version_type;
        QString changelog;
        ModPlatform::ResourceProvider provider;
        shared_qobject_ptr<ResourceDownloadTask> download;
        bool enabled = true;

       public:
        Update(QString name,
               QString old_h,
               QString old_v,
               QString new_v,
               std::optional<ModPlatform::IndexedVersionType> new_v_type,
               QString changelog,
               ModPlatform::ResourceProvider p,
               shared_qobject_ptr<ResourceDownloadTask> t,
               bool enabled = true)
            : name(name)
            , old_hash(old_h)
            , old_version(old_v)
            , new_version(new_v)
            , new_version_type(new_v_type)
            , changelog(changelog)
            , provider(p)
            , download(t)
            , enabled(enabled)
        {}
    };

    auto getUpdates() -> std::vector<Update>&& { return std::move(m_updates); }
    auto getDependencies() -> QList<std::shared_ptr<GetModDependenciesTask::PackDependency>>&& { return std::move(m_deps); }

   public slots:
    bool abort() override = 0;

   protected slots:
    void executeTask() override = 0;

   signals:
    void checkFailed(Resource* failed, QString reason, QUrl recover_url = {});

   protected:
    QList<Resource*>& m_resources;
    std::list<Version>& m_game_versions;
    std::optional<ModPlatform::ModLoaderTypes> m_loaders;
    std::shared_ptr<ResourceFolderModel> m_resource_model;

    std::vector<Update> m_updates;
    QList<std::shared_ptr<GetModDependenciesTask::PackDependency>> m_deps;
};

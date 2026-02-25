#include "components/cleanup_job.hpp"
#include "components/metadata_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;

namespace write_service {

CleanupJob::CleanupJob(const components::ComponentConfig& config,
                       const components::ComponentContext& ctx)
    : components::ComponentBase(config, ctx)
    , blob_repo_(ctx.FindComponent<BlobRepo>())
    , pg_(ctx.FindComponent<components::Postgres>(MetadataRepo::kDefaultPgComponent).GetCluster())
    , batch_size_expired_(config["batch_size_expired"].As<int32_t>(50))
    , batch_size_deleted_(config["batch_size_deleted"].As<int32_t>(50))
{
    const auto interval = std::chrono::seconds{config["interval_s"].As<int>(120)};
    task_.Start("cleanup_job", {interval}, [this] { Run(); });
}

CleanupJob::~CleanupJob() {
    task_.Stop();
}

void CleanupJob::Run() {
    try {
        CleanupDeleted();
    } catch (const std::exception& e) {
        LOG_ERROR() << "CleanupDeleted failed: " << e.what();
    }

    try {
        CleanupExpired();
    } catch (const std::exception& e) {
        LOG_ERROR() << "CleanupExpired failed: " << e.what();
    }
}

void CleanupJob::CleanupDeleted() {
    auto result = pg_->Execute(
        storages::postgres::ClusterHostType::kMaster,
        "DELETE FROM pastes.metadata "
        "WHERE status = 'deleted' "
        "LIMIT $1 "
        "RETURNING id, s3_version_id",
        batch_size_deleted_
    );
    if (result.IsEmpty()) return;

    LOG_INFO() << "CleanupDeleted: found " << result.Size() << " pastes";

    for (const auto& row : result) {
        auto paste_id = row["id"].As<std::string>();
        auto version_id = row["s3_version_id"].As<std::optional<std::string>>();

        auto delete_result = userver::utils::Async("cleanup_s3_delete", [this, &paste_id, &version_id] {
            return blob_repo_.DeletePasteBlobBlocking("submitted/" + paste_id, version_id);
        }).Get();

        if (!delete_result) {
            LOG_WARNING() << "CleanupDeleted: failed to delete s3 object paste_id=" << paste_id;
        }
    }
}

void CleanupJob::CleanupExpired() {
    auto result = pg_->Execute(
        storages::postgres::ClusterHostType::kMaster,
        "DELETE FROM pastes.metadata "
        "WHERE status = 'submitted' AND expires_at < NOW() "
        "LIMIT $1 "
        "RETURNING id, s3_version_id",
        batch_size_expired_
    );
    if (result.IsEmpty()) return;

    LOG_INFO() << "CleanupExpired: found " << result.Size() << " pastes";

    for (const auto& row : result) {
        auto paste_id = row["id"].As<std::string>();
        auto version_id = row["s3_version_id"].As<std::optional<std::string>>();

        auto delete_result = userver::utils::Async("cleanup_s3_expire", [this, &paste_id, &version_id] {
            return blob_repo_.DeletePasteBlobBlocking("submitted/" + paste_id, version_id);
        }).Get();

        if (!delete_result) {
            LOG_WARNING() << "CleanupExpired: failed to delete s3 object paste_id=" << paste_id;
        }
    }
}

userver::yaml_config::Schema CleanupJob::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<ComponentBase>(R"(
        type: object
        description: Periodic cleanup job
        additionalProperties: false
        properties:
            interval_s:
                type: integer
                description: Cleanup interval in seconds
                defaultDescription: 120
            batch_size_expired:
                type: integer
                description: How many expired pastes can be deleted at once
                defaultDescription: 50
            batch_size_deleted:
                type: integer
                description: How many soft-deleted pastes can be deleted at once
                defaultDescription: 50
    )");
}

}
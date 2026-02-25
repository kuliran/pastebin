#include "components/metadata_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

using namespace userver;

namespace write_service {

MetadataRepo::MetadataRepo(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , pg_cluster_(component_context.FindComponent<components::Postgres>(kDefaultPgComponent).GetCluster())
    , rate_limit_window_duration_s_(config["upload_rate_limit_window_duration_s"].As<std::int32_t>())
    , create_url_limit_(config["upload_rate_limit_create_url_max"].As<std::int32_t>())
    , submit_limit_(config["upload_rate_limit_submit_max"].As<std::int32_t>())
{}

std::optional<CreatePendingUploadError> MetadataRepo::CreatePendingUpload(const PasteMetadata& metadata) const {
    try {
        LOG_DEBUG() << "creating upload url user_id=" << metadata.owner_user_id;
        auto transaction = pg_cluster_->Begin(
            storages::postgres::ClusterHostType::kMaster,
            storages::postgres::TransactionOptions{}
        );

        auto rate_limit_result = transaction.Execute(R"~~~(
            WITH params AS (
                SELECT make_interval(secs => $2) AS window_duration
            )
            INSERT INTO pastes.rate_limit AS rl (user_id, upload_create_url_cnt, window_started_at)
            VALUES ($1, 1, NOW())
            ON CONFLICT (user_id) DO UPDATE
            SET upload_create_url_cnt = 
                CASE 
                    WHEN rl.window_started_at < NOW() - (SELECT window_duration FROM params) 
                    THEN 1
                    ELSE rl.upload_create_url_cnt + 1
                END,
                window_started_at = 
                CASE 
                    WHEN rl.window_started_at < NOW() - (SELECT window_duration FROM params) 
                    THEN NOW()
                    ELSE rl.window_started_at
                END
            RETURNING rl.upload_submit_cnt, rl.upload_create_url_cnt)~~~",
            metadata.owner_user_id,
            rate_limit_window_duration_s_
        );
        auto [upload_submit_cnt, upload_create_url_cnt] = rate_limit_result.AsSingleRow<std::tuple<int32_t, int32_t>>(storages::postgres::kRowTag);
        if (upload_submit_cnt > submit_limit_ || upload_create_url_cnt > create_url_limit_) {
            LOG_DEBUG() << "rate limit exceeded: submit_cnt=" << upload_submit_cnt << " create_url_cnt=" << upload_create_url_cnt;
            return {CreatePendingUploadError::kUserRateLimitExceeded};
        }

        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO pastes.metadata "
            "(id, owner_user_id, created_at, expires_at, size_bytes, status) "
            "VALUES ($1, $2, $3, $4, $5, 'pending') "
            "ON CONFLICT (id) DO NOTHING",
            metadata.id,
            metadata.owner_user_id,
            metadata.created_at,
            metadata.expires_at,
            metadata.size_bytes
        );
        if (result.RowsAffected() == 0)
            return {CreatePendingUploadError::kIdCollision};

        transaction.Commit();
        return std::nullopt;
    } catch(const storages::postgres::UniqueViolation& e) {
        // In case of race condition
        LOG_WARNING() << "DB unique violation: " << e.what();
        return {CreatePendingUploadError::kIdCollision};
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {CreatePendingUploadError::kDbError};
    }
}

std::optional<SubmitUploadMetadataError> MetadataRepo::SubmitUpload(std::string_view paste_id, std::string_view version_id,
    std::string_view user_id, std::int32_t size_bytes) const {
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,
            "UPDATE pastes.metadata "
            "SET status = 'submitted', s3_version_id = $1, size_bytes = $2 "
            "WHERE id = $3 AND owner_user_id = $4 AND status = 'pending'",
            version_id,
            size_bytes,
            paste_id,
            user_id
        );

        if (result.RowsAffected() == 0) {
            return {SubmitUploadMetadataError::kConflict};
        }
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {SubmitUploadMetadataError::kDbError};
    }

    return std::nullopt;
}

std::optional<DeletePasteMetadataError>
    MetadataRepo::DeletePasteMetadata(const std::string_view& id, const std::string_view& user_id) const {
    try {
        auto transaction = pg_cluster_->Begin(
            storages::postgres::ClusterHostType::kMaster,
            storages::postgres::TransactionOptions{}
        );

        auto result = transaction.Execute(
            "SELECT owner_user_id, status "
            "FROM pastes.metadata "
            "WHERE id = $1",
            id
        );
        if (result.IsEmpty())
            return {DeletePasteMetadataError::kNotExists};

        auto [owner_user_id, status] = result.AsSingleRow<std::tuple<std::string, PasteStatus>>(storages::postgres::kRowTag);
        if (user_id != owner_user_id)
            return {DeletePasteMetadataError::kUnauthorized};
        if (status == PasteStatus::kSoftDeleted)
            return {DeletePasteMetadataError::kAlreadySoftDeleted};

        transaction.Execute(
            "UPDATE pastes.metadata "
            "SET status = 'deleted' "
            "WHERE id = $1",
            id
        );
        transaction.Commit();
        return std::nullopt;
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {DeletePasteMetadataError::kDbError};
    }

    return std::nullopt;
}

userver::yaml_config::Schema MetadataRepo::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<LoggableComponentBase>(R"(
        type: object
        description: Paste metadata repo, responsible for metadata DB storage
        additionalProperties: false
        properties:
            upload_rate_limit_window_duration_s:
                type: integer
                description: Duration of rate limit window before it gets reset (in seconds)
            upload_rate_limit_submit_max:
                type: integer
                description: Number of calls 1 user is allowed to make to submit an upload within a rate limit window
            upload_rate_limit_create_url_max:
                type: integer
                description: If a user has already exceeded the submit max, they cannot create an upload url regardless of this value
        )"
    );
}

}
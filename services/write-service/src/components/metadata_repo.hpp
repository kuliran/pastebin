#pragma once

#include "models/paste_metadata.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/expected.hpp>

namespace write_service {

enum class CreatePendingUploadError {
    kIdCollision,
    kUserRateLimitExceeded,
    kDbError,
};

enum class SubmitUploadMetadataError {
    kConflict,
    kDbError,
};

enum class DeletePasteMetadataError {
    kNotExists,
    kAlreadySoftDeleted,
    kUnauthorized,
    kDbError,
};

class MetadataRepo : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "metadata-repo";
    static constexpr std::string_view kDefaultPgComponent = "postgres-db-1";

    MetadataRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    std::optional<CreatePendingUploadError>
        CreatePendingUpload(const PasteMetadata& metadata) const;

    std::optional<SubmitUploadMetadataError> SubmitUpload(std::string_view paste_id, std::string_view version_id,
        std::string_view user_id, std::int32_t size_bytes) const;

    std::optional<DeletePasteMetadataError>
        DeletePasteMetadata(const std::string_view& id, const std::string_view& user_id) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();
private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    std::int32_t rate_limit_window_duration_s_;
    std::int32_t create_url_limit_;
    std::int32_t submit_limit_;
};
    
}
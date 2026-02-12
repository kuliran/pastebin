#pragma once

#include "models/paste_metadata.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/expected.hpp>

namespace paste_service {

enum class GetPasteMetadataError {
    kNotFound,
    kSoftExpired,
    kDbError
};

enum class UploadPasteMetadataError {
    kIdCollision,
    kDbError,
};

enum class DeletePasteMetadataError {
    kNotExists,
    kDbError,
};

class MetadataRepo : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "metadata-repo";

    MetadataRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<PasteMetadata, GetPasteMetadataError>
        GetPasteMetadata(const std::string_view& id) const;

    std::optional<UploadPasteMetadataError>
        UploadPasteMetadata(const PasteMetadata& metadata) const;

    std::optional<DeletePasteMetadataError>
        DeletePasteMetadata(const std::string_view& id, const std::string_view& delete_key) const;
private:
    static constexpr std::string_view kDefaultPgComponent = "postgres-db-1";

    userver::storages::postgres::ClusterPtr pg_cluster_;
};
    
}
#pragma once

#include "models/paste_metadata.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/expected.hpp>

namespace read_service {

enum class GetPasteMetadataError {
    kNotFound,
    kUnauthorized,
    kSoftExpired,
    kDbError,
};

class MetadataRepo : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "metadata-repo";

    MetadataRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<PasteMetadata, GetPasteMetadataError>
        GetPasteMetadata(std::string_view id, std::string_view user_id) const;
private:
    static constexpr std::string_view kDefaultPgComponent = "postgres-db-1";

    userver::storages::postgres::ClusterPtr pg_cluster_;
};
    
}
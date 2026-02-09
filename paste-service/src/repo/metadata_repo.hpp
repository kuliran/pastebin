#pragma once

#include "repo/paste_metadata.hpp"

#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/expected.hpp>

namespace paste_service {

enum class GetPasteMetadataError {
    kNotFound,
    kSoftExpired,
    kDbError
};

class MetadataRepo {
public:
    MetadataRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<PasteMetadata, GetPasteMetadataError>
        GetPasteMetadata(const std::string_view& id) const;

private:
    static constexpr std::string_view kDefaultPgComponent = "postgres-db-1";

    userver::storages::postgres::ClusterPtr pg_cluster_;
};
    
}
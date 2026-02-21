#include "components/metadata_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;

namespace read_service {

MetadataRepo::MetadataRepo(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , pg_cluster_(component_context.FindComponent<components::Postgres>(kDefaultPgComponent).GetCluster())
{}

utils::expected<PasteMetadata, GetPasteMetadataError>
    MetadataRepo::GetPasteMetadata(const std::string_view& id) const {
    
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kSlave,
            "SELECT * "
            "FROM pastes.metadata "
            "WHERE id = $1",
            id
        );
        if (result.IsEmpty()) {
            return {GetPasteMetadataError::kNotFound};
        }

        auto metadata = result.AsSingleRow<PasteMetadata>(storages::postgres::kRowTag);
        if (std::chrono::system_clock::now() >= metadata.expires_at) {
            return {GetPasteMetadataError::kSoftExpired};
        }

        return {metadata};
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {GetPasteMetadataError::kDbError};
    }
}

}
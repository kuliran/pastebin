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
    MetadataRepo::GetPasteMetadata(std::string_view id, std::string_view user_id) const {
    
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kSlave,
            "SELECT id, owner_user_id, created_at, expires_at, size_bytes "
            "FROM pastes.metadata "
            "WHERE id = $1 AND status = 'submitted'",
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
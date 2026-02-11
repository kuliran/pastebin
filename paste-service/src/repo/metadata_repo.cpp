#include "repo/metadata_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;

namespace paste_service {

MetadataRepo::MetadataRepo(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : pg_cluster_(component_context.FindComponent<components::Postgres>(
        config["postgres-component"].As<std::string>(kDefaultPgComponent)
    ).GetCluster())
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

std::optional<UploadPasteMetadataError> MetadataRepo::UploadPasteMetadata(const PasteMetadata& metadata) const {
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,
            "INSERT INTO pastes.metadata "
            "(id, created_at, expires_at, size_bytes, delete_key) "
            "VALUES ($1, $2, $3, $4, $5) "
            "ON CONFLICT (id) DO NOTHING",
            metadata.id,
            metadata.created_at,
            metadata.expires_at,
            metadata.size_bytes,
            metadata.delete_key
        );

        if (result.RowsAffected() == 0) {
            return {UploadPasteMetadataError::kIdCollision};
        }
    } catch(const storages::postgres::UniqueViolation& e) {
        // In case of race condition
        LOG_WARNING() << "DB unique violation: " << e.what();
        return {UploadPasteMetadataError::kIdCollision};
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {UploadPasteMetadataError::kDbError};
    }

    return std::nullopt;
}

}
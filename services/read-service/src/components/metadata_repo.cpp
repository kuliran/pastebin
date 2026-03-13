#include "components/metadata_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;

namespace read_service {

MetadataRepo::MetadataRepo(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , pg_cluster_(component_context.FindComponent<components::Postgres>(kDefaultPgComponent).GetCluster())
{}

utils::expected<PasteMetadata, GetPasteMetadataError> MetadataRepo::GetPasteMetadata(std::string_view id) const {
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kSlave,
            "SELECT id, owner_user_id, created_at, expires_at, visibility, size_bytes "
            "FROM pastes.metadata "
            "WHERE id = $1 AND status = 'submitted'",
            id
        );
        if (result.IsEmpty()) {
            return {GetPasteMetadataError::kNotFound};
        }

        return {result.AsSingleRow<PasteMetadata>(storages::postgres::kRowTag)};
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {GetPasteMetadataError::kDbError};
    }
}

bool MetadataRepo::UserHasAccessToPrivatePaste(std::string_view id, std::string_view user_id) const {
    const auto result = pg_cluster_->Execute(
        storages::postgres::ClusterHostType::kSlave,
        "SELECT 1 "
        "FROM pastes.private_permissions "
        "WHERE paste_id = $1 AND user_id = $2",
        id, user_id
    );
    return !result.IsEmpty();
}

userver::utils::expected<dto::PastePrivatePermsUserIds, GetPastePrivatePermsUserIdsError>
MetadataRepo::GetPastePrivatePermsUserIds(std::string_view id) const {
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kSlave,
            "SELECT user_id "
            "FROM pastes.private_permissions "
            "WHERE paste_id = $1",
            id
        );

        return {result.AsContainer<dto::PastePrivatePermsUserIds>()};
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {GetPastePrivatePermsUserIdsError::kDbError};
    }
}

userver::utils::expected<dto::GetUserPastesResult, dto::GetUserPastesError>
MetadataRepo::GetUserPastes(std::string_view user_id) const {
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,
            "SELECT id, visibility, created_at "
            "FROM pastes.metadata "
            "WHERE owner_user_id = $1 AND status = 'submitted'",
            user_id
        );
        return result.AsContainer<dto::GetUserPastesResult>(storages::postgres::kRowTag);
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {dto::GetUserPastesError::kDbError};
    }
}

}
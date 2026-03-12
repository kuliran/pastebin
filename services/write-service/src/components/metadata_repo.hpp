#pragma once

#include "models/paste_metadata.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/expected.hpp>

namespace write_service {

enum class CreatePendingUploadError { kIdCollision, kUserRateLimitExceeded, kDbError };
enum class SubmitUploadMetadataError { kConflict, kDbError };
enum class DeletePasteMetadataError { kAlreadySoftDeleted, kDbError };
enum class IsPasteOwnerError { kNotExists, kNotOwner, kDbError };
enum class PatchPasteVisibilityRepoError { kNotExists, kDbError };
enum class PastePrivatePermsAddRepoError { kDbError };
using PastePrivatePermsRmRepoError = PastePrivatePermsAddRepoError;

class MetadataRepo : public userver::components::LoggableComponentBase {
public:
    using UnitOfWork = userver::storages::postgres::Transaction;

    static constexpr std::string_view kName = "metadata-repo";
    static constexpr std::string_view kDefaultPgComponent = "postgres-db-1";

    MetadataRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    UnitOfWork BeginUnit();

    std::optional<CreatePendingUploadError> CreatePendingUpload(UnitOfWork& u, const PasteMetadata& metadata) const;
    std::optional<SubmitUploadMetadataError> SubmitUpload(
        std::string_view paste_id, std::string_view version_id,
        std::string_view user_id, std::int32_t size_bytes
    ) const;
    std::optional<IsPasteOwnerError> IsPasteOwner(std::string_view id, std::string_view user_id) const;
    std::optional<DeletePasteMetadataError> DeletePasteMetadata(std::string_view id) const;
    std::optional<PatchPasteVisibilityRepoError> PatchPasteVisibility(UnitOfWork&, std::string_view id, PasteVisibility visibility) const;
    std::optional<PastePrivatePermsAddRepoError> PastePrivatePermsAdd(UnitOfWork&, std::string_view id, std::vector<std::string> users) const;
    std::optional<PastePrivatePermsRmRepoError> PastePrivatePermsRm(UnitOfWork&, std::string_view id, std::vector<std::string> users) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();
private:
    userver::storages::postgres::ClusterPtr pg_cluster_;
    std::int32_t rate_limit_window_duration_s_;
    std::int32_t create_url_limit_;
    std::int32_t submit_limit_;
};
    
}
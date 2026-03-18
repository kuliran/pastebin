#include "services/read_service.hpp"
#include "components/friends_client_component.hpp"

#include <userver/components/component.hpp>
#include <userver/utils/uuid4.hpp>
#include <userver/tracing/span.hpp>

using namespace userver;
using namespace read_service::dto;

namespace read_service {

ReadService::ReadService(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , metadata_repo_(component_context.FindComponent<MetadataRepo>(MetadataRepo::kName))
    , blob_repo_(component_context.FindComponent<BlobRepo>(BlobRepo::kName))
    , friends_client_(component_context.FindComponent<FriendsClientComponent>(FriendsClientComponent::kName).GetClientWrapper())
{}

utils::expected<GetPasteResult, GetPasteError> ReadService::GetPaste(std::string_view id, std::string_view user_id) const {
    auto metadata = metadata_repo_.GetPasteMetadata(id);
    if (!metadata) {
        switch (metadata.error()) {
            case GetPasteMetadataError::kNotFound: return {GetPasteError::kNotExists};
            case GetPasteMetadataError::kDbError: return {GetPasteError::kDbError};
        }
    }

    if (std::chrono::system_clock::now() >= metadata.value().expires_at) {
        return {GetPasteError::kSoftExpired};
    }

    if (user_id != metadata.value().owner_user_id) {
        if (metadata.value().visibility == PasteVisibility::kFriends) {
            if (!friends_client_.AreFriends(
                std::string(user_id),
                metadata.value().owner_user_id
            )) {
                return {GetPasteError::kUnauthorized};
            }
        } else if (metadata.value().visibility == PasteVisibility::kPrivate) {
            if (!metadata_repo_.UserHasAccessToPrivatePaste(id, user_id)) {
                return {GetPasteError::kUnauthorized};
            }
        }
    }

    auto presigned_url = blob_repo_.CreatePresignedGet(id, kPresignedGetUrlTtl);
    return GetPasteResult(std::move(metadata.value()), std::move(presigned_url));
}

userver::utils::expected<GetPasteDetailsResult, GetPasteDetailsError>
ReadService::GetPasteDetails(std::string_view id, std::string_view user_id) const {
    auto metadata = metadata_repo_.GetPasteMetadata(id);
    if (!metadata) {
        switch (metadata.error()) {
            case GetPasteMetadataError::kNotFound: return {GetPasteDetailsError::kNotExists};
            case GetPasteMetadataError::kDbError: return {GetPasteDetailsError::kDbError};
        }
    }

    if (user_id != metadata.value().owner_user_id) return {GetPasteDetailsError::kUnauthorized};
    if (metadata.value().visibility != PasteVisibility::kPrivate) {
        return GetPasteDetailsResult();
    }

    auto private_perms_user_ids = metadata_repo_.GetPastePrivatePermsUserIds(id);
    if (!private_perms_user_ids) {
        switch (private_perms_user_ids.error()) {
            case GetPastePrivatePermsUserIdsError::kDbError: return {GetPasteDetailsError::kDbError};
        }
    }

    return GetPasteDetailsResult(std::move(metadata.value()), std::move(private_perms_user_ids.value()));
}

userver::utils::expected<GetUserPastesResult, GetUserPastesError>
ReadService::GetUserPastes(std::string_view user_id) const {
    auto result = metadata_repo_.GetUserPastes(user_id);
    if (!result) {
        switch (result.error()) {
            case GetUserPastesError::kDbError: return {GetUserPastesError::kDbError};
        }
    }
    return result;
}

}
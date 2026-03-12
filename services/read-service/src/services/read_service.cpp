#include "services/read_service.hpp"

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
{}

utils::expected<GetPasteResult, GetPasteError> ReadService::GetPaste(std::string_view id, std::string_view user_id) const {
    const auto metadata = metadata_repo_.GetPasteMetadata(id);
    if (!metadata) {
        switch (metadata.error()) {
            case GetPasteMetadataError::kNotFound:
                return {GetPasteError::kNotExists};
            case GetPasteMetadataError::kDbError:
                return {GetPasteError::kDbError};
        }
    }

    if (std::chrono::system_clock::now() >= metadata.value().expires_at) {
        return {GetPasteError::kSoftExpired};
    }

    if (metadata.value().visibility == PasteVisibility::kFriends) {
        // TODO
        return {GetPasteError::kUnauthorized};
    } else if (metadata.value().visibility == PasteVisibility::kPrivate) {
        if (user_id != metadata.value().owner_user_id
            && !metadata_repo_.UserHasAccessToPrivatePaste(id, user_id)) {
            return {GetPasteError::kUnauthorized};
        }
    }

    auto presigned_url = blob_repo_.CreatePresignedGet(id, kPresignedGetUrlTtl);
    return GetPasteResult(std::move(metadata.value()), std::move(presigned_url));
}

}
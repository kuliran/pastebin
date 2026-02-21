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

utils::expected<GetPasteResult, GetPasteError> ReadService::GetPaste(const std::string_view& id) const {
    const auto metadata = metadata_repo_.GetPasteMetadata(id);
    if (!metadata) {
        switch (metadata.error()) {
            case GetPasteMetadataError::kNotFound:
                return {GetPasteError::kNotExists};
            case GetPasteMetadataError::kSoftExpired:
                return {GetPasteError::kSoftExpired};
            default:
                return {GetPasteError::kDbError};
        }
    }

    const auto blob = blob_repo_.GetPasteBlob(id);
    if (!blob) {
        switch (blob.error()) {
            case read_service::GetPasteBlobError::kNotFound: {
                LOG_WARNING() << "Paste metadata exists, but no blob; paste_id=" << id;
                return {GetPasteError::kNotExists};
            }
            default: {
                return {GetPasteError::kDbError};
            }
        }
    }
    return GetPasteResult(std::move(metadata.value()), std::move(blob.value()));
}

}
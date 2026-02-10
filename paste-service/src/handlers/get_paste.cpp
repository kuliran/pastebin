#include "handlers/get_paste.hpp"
#include "dto/paste_response.hpp"

using namespace userver;

namespace paste_service {

GetPaste::GetPaste(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , metadata_repo_(config, component_context)
    , blob_repo_(config, component_context)
{}

formats::json::Value GetPaste::
    HandleRequestJsonThrow(const HttpRequest& request, const Value&, RequestContext&)
        const {
    using userver::server::http::HttpStatus;

    const auto& id = request.GetPathArg(kPathArgId);
    if (id.empty() || id.size() > 128) {
        request.SetResponseStatus(HttpStatus::BadRequest);
        return {};
    }

    auto span = tracing::Span::CurrentSpan().CreateChild("get_paste");
    span.AddTag("paste_id", std::string(id));

    const auto metadata = metadata_repo_.GetPasteMetadata(id);
    if (!metadata) {
        switch (metadata.error()) {
            case GetPasteMetadataError::kNotFound:
            case GetPasteMetadataError::kSoftExpired: {
                request.SetResponseStatus(HttpStatus::NotFound);
                return {};
            }
            default: {
                request.SetResponseStatus(HttpStatus::InternalServerError);
                return {};
            }
        }
    }

    const auto blob = blob_repo_.GetPasteBlob(id);
    if (!blob) {
        switch (blob.error()) {
            case paste_service::GetPasteBlobError::kNotFound: {
                LOG_ERROR() << "Paste metadata exists, but no blob: " << id;
                request.SetResponseStatus(HttpStatus::kNotFound);
                return {};
            }
            default: {
                request.SetResponseStatus(HttpStatus::InternalServerError);
                return {};
            }
        }
    }

    auto res = dto::MakePasteResponse(metadata.value(), blob.value());
    return formats::json::ValueBuilder(res).ExtractValue();
}

}  // namespace paste_service
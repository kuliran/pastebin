#include "handlers/upload_paste.hpp"
#include "utils/id_gen.hpp"
#include "dto/upload_paste_response.hpp"

#include <userver/utils/uuid4.hpp>
#include <userver/formats/json.hpp>

using namespace userver;

namespace paste_service {

UploadPaste::UploadPaste(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , metadata_repo_(config, component_context)
    , blob_repo_(config, component_context)
{}

formats::json::Value UploadPaste::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext&)
        const {
    using userver::server::http::HttpStatus;

    if (!request_json.IsObject() || !request_json.HasMember("text") || !request_json["text"].IsString()) {
        request.SetResponseStatus(HttpStatus::kBadRequest);
        return {};
    }

    std::string text = request_json["text"].As<std::string>();
    if (text.empty()) {
        request.SetResponseStatus(HttpStatus::kBadRequest);
        return {};
    }
    // size limit is also set in static_config.yaml to be slightly more than here
    // to permit json payload (which adds a bit of overhead)
    if (text.size() > kMaxBlobSizeBytes) {
        request.SetResponseStatus(HttpStatus::kPayloadTooLarge);
        return {};
    }

    std::string delete_key = utils::generators::GenerateUuid();
    auto now = std::chrono::system_clock::now();
    auto expires_at = now + std::chrono::hours(24*7);

    PasteBlob blob{{}, std::move(text)};
    PasteMetadata metadata{
        .id = {}, // is set below
        .created_at = storages::postgres::TimePointTz(now),
        .expires_at = storages::postgres::TimePointTz(expires_at),
        .delete_key = std::move(delete_key),
        .size_bytes = static_cast<int>(blob.text.size()),
    };

    for (int i = 0; i <= kIdCollisionRetries; ++i) {
        blob.id = id_gen::GenId();

        auto span = tracing::Span::CurrentSpan().CreateChild("upload_paste");
        span.AddTag("paste_id", blob.id);

        auto blob_err = blob_repo_.UploadPasteBlob(blob);
        if (blob_err) {
            if (*blob_err == UploadPasteBlobError::kIdCollision)
                continue;
            
            request.SetResponseStatus(HttpStatus::kInternalServerError);
            return {};
        }

        metadata.id = std::move(blob.id);
        auto metadata_err = metadata_repo_.UploadPasteMetadata(metadata);
        if (metadata_err) {
            if (*metadata_err == UploadPasteMetadataError::kIdCollision)
                continue;
            
            request.SetResponseStatus(HttpStatus::kInternalServerError);
            return {};
        }

        auto res = dto::MakeUploadPasteResponse(metadata);
        return formats::json::ValueBuilder(res).ExtractValue();
    }
    
    LOG_WARNING() << "Upload id gen exceeded the number of retries";
    request.SetResponseStatus(HttpStatus::kInternalServerError);
    return {};
}

}  // namespace paste_service
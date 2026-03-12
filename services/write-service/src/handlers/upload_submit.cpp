#include "handlers/upload_submit.hpp"

using namespace userver;

namespace write_service {

UploadSubmit::UploadSubmit(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , write_service_(component_context.FindComponent<WriteService>(WriteService::kName))
{}

formats::json::Value UploadSubmit::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext& ctx)
        const {
    using userver::server::http::HttpStatus;

    if (!request_json.IsObject() || !request_json.HasMember("paste_id") || !request_json["paste_id"].IsString()) {
        request.SetResponseStatus(HttpStatus::kBadRequest); return {};
    }

    auto paste_id = request_json["paste_id"].As<std::string>();
    const std::string& user_id = ctx.GetData<std::string>("user_id");

    auto span = tracing::Span::CurrentSpan().CreateChild("upload_submit_http");
    span.AddTag("user_id", user_id);
    span.AddTag("paste_id", paste_id);

    auto result = write_service_.SubmitUpload(paste_id, user_id);
    if (!result) {
        switch (result.error()) {
            case dto::SubmitUploadError::kBlobTooLarge: { request.SetResponseStatus(HttpStatus::kPayloadTooLarge); return {}; }
            case dto::SubmitUploadError::kBadBlobContent:
            case dto::SubmitUploadError::kBlobNotExists:
            case dto::SubmitUploadError::kConflict: { request.SetResponseStatus(HttpStatus::kConflict); return {}; }
            case dto::SubmitUploadError::kDbError: { request.SetResponseStatus(HttpStatus::kInternalServerError); return {}; }
        }
    }

    return {};
}

}  // namespace write_service
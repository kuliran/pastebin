#include "handlers/patch_paste.hpp"
#include "utils/handle_paste_privacy_json.hpp"

using namespace userver;

namespace write_service {

PatchPaste::PatchPaste(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , write_service_(component_context.FindComponent<WriteService>(WriteService::kName))
{}

formats::json::Value PatchPaste::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext& ctx)
        const {
    using userver::server::http::HttpStatus;
    if (!request_json.IsObject()) { request.SetResponseStatus(HttpStatus::kBadRequest); return {}; }

    auto privacy_settings = HandlePastePrivacyJson(request_json);
    if (!privacy_settings) { request.SetResponseStatus(HttpStatus::kBadRequest); return {}; }

    const auto& id = request.GetPathArg("id");
    const std::string& user_id = ctx.GetData<std::string>("user_id");
    auto span = tracing::Span::CurrentSpan().CreateChild("patch_paste");
    span.AddTag("user_id", user_id);

    auto res = write_service_.PatchPastePrivacy(id, user_id, std::move(privacy_settings.value()));
    if (!res) {
        switch (res.error()) {
            case dto::PatchPastePrivacyError::kEmptyRequest: { request.SetResponseStatus(HttpStatus::kBadRequest); return {}; }
            case dto::PatchPastePrivacyError::kNotExists: { request.SetResponseStatus(HttpStatus::kNotFound); return {}; }
            case dto::PatchPastePrivacyError::kUnauthorized: { request.SetResponseStatus(HttpStatus::kForbidden); return {}; }
            case dto::PatchPastePrivacyError::kDbError: { request.SetResponseStatus(HttpStatus::kInternalServerError); return {}; }
        }
    }

    return {};
}

}  // namespace write_service
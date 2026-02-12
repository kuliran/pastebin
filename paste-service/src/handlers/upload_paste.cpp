#include "handlers/upload_paste.hpp"
#include "services/dto/paste_dto_json.hpp" // IWYU pragma: keep; ADL json Serialize provider

using namespace userver;

namespace paste_service {

UploadPaste::UploadPaste(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , paste_service_(component_context.FindComponent<PasteService>(PasteService::kName))
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

    auto span = tracing::Span::CurrentSpan().CreateChild("upload_paste_http");

    auto result = paste_service_.UploadPaste(std::move(text));
    if (!result) {
        switch (result.error()) {
            case dto::UploadPasteError::kEmptyText: {
                request.SetResponseStatus(HttpStatus::kBadRequest);
                return {};
            }
            case dto::UploadPasteError::kTextTooLarge: {
                // size limit is also set in static_config.yaml to be slightly more than here
                // to permit json payload (which adds a bit of overhead)
                request.SetResponseStatus(HttpStatus::kPayloadTooLarge);
                return {};
            }
            default: {
                request.SetResponseStatus(HttpStatus::kInternalServerError);
                return {};
            }
        }
    }

    return formats::json::ValueBuilder(std::move(result.value())).ExtractValue();
}

}  // namespace paste_service
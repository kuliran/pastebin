#include "handlers/upload_paste.hpp"
#include "services/dto/paste_dto_json.hpp" // IWYU pragma: keep; ADL json Serialize provider

using namespace userver;

namespace write_service {

inline static const std::unordered_map<std::string_view, dto::UploadPasteLifetime> kLifetimeMap {
    {"1_hour", dto::UploadPasteLifetime::k1Hour},
    {"1_day", dto::UploadPasteLifetime::k1Day},
    {"1_week", dto::UploadPasteLifetime::k1Week},
    {"1_month", dto::UploadPasteLifetime::k1Month},
    {"3_month", dto::UploadPasteLifetime::k3Months},
};

UploadPaste::UploadPaste(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , write_service_(component_context.FindComponent<WriteService>(WriteService::kName))
{}

formats::json::Value UploadPaste::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext&)
        const {
    using userver::server::http::HttpStatus;

    if (!request_json.IsObject() || !request_json.HasMember("text") || !request_json["text"].IsString()
        || (request_json.HasMember("expires_in") && !request_json["expires_in"].IsString())) {
        request.SetResponseStatus(HttpStatus::kBadRequest);
        return {};
    }
    std::string text = request_json["text"].As<std::string>();
    dto::UploadPasteLifetime lifetime;

    auto expires_in = request_json["expires_in"].As<std::optional<std::string>>();
    if (!expires_in) {
        lifetime = dto::UploadPasteLifetime::k1Week;
    } else {
        auto it = kLifetimeMap.find(*expires_in);
        if (it == kLifetimeMap.end()) {
            request.SetResponseStatus(HttpStatus::kBadRequest);
            return {};
        }
        lifetime = it->second;
    }

    auto span = tracing::Span::CurrentSpan().CreateChild("upload_paste_http");

    auto result = write_service_.UploadPaste(std::move(text), lifetime);
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
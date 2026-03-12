#include "handlers/upload_create_url.hpp"
#include "utils/handle_paste_privacy_json.hpp"
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

UploadCreateUrl::UploadCreateUrl(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , write_service_(component_context.FindComponent<WriteService>(WriteService::kName))
{}

formats::json::Value UploadCreateUrl::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext& ctx)
        const {
    using userver::server::http::HttpStatus;
    if (!request_json.IsObject()) { request.SetResponseStatus(HttpStatus::kBadRequest); return {}; }

    dto::UploadPasteLifetime lifetime;
    if (auto param = request_json["expires_in"]; param.IsString()) {
        auto v = param.As<std::string>();
        auto it = kLifetimeMap.find(v);
        if (it == kLifetimeMap.end()) { request.SetResponseStatus(HttpStatus::kBadRequest); return {}; }
        lifetime = it->second;
    } else {
        lifetime = dto::UploadPasteLifetime::k1Week;
    }
    
    auto privacy_settings = HandlePastePrivacyJson(request_json);
    if (!privacy_settings) { request.SetResponseStatus(HttpStatus::kBadRequest); return {}; }

    const std::string& user_id = ctx.GetData<std::string>("user_id");
    auto span = tracing::Span::CurrentSpan().CreateChild("upload_create_url_http");
    span.AddTag("user_id", user_id);

    auto result = write_service_.CreateUploadPresignedUrl(user_id, lifetime, std::move(privacy_settings.value()));
    if (!result) {
        switch (result.error()) {
            case dto::CreateUploadPresignedUrlError::kInvalidLifetimeParam: { request.SetResponseStatus(HttpStatus::kBadRequest); return {}; }
            case dto::CreateUploadPresignedUrlError::kUserRateLimitExceeded: {
                request.SetResponseStatus(HttpStatus::kTooManyRequests);
                return formats::json::MakeObject("msg", "paste upload rate limit exceeded");
            }
            case dto::CreateUploadPresignedUrlError::kIdCollisionRetryExceeded:
            case dto::CreateUploadPresignedUrlError::kDbError: { request.SetResponseStatus(HttpStatus::kInternalServerError); return {}; }
        }
    }

    request.SetResponseStatus(HttpStatus::kCreated);
    return formats::json::ValueBuilder(std::move(result.value())).ExtractValue();
}

}  // namespace write_service
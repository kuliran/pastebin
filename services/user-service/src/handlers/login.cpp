#include "handlers/login.hpp"
#include "services/dto/user_dto.hpp"

#include <userver/formats/json.hpp>

using namespace userver;

namespace user_service {

Login::Login(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , user_service_(component_context.FindComponent<UserService>(UserService::kName))
    , cookie_factory_(component_context.FindComponent<CookieFactory>(CookieFactory::kName))
{}

formats::json::Value Login::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext&)
        const {
    using userver::server::http::HttpStatus;
    using namespace user_service::dto;

    if (!request_json.IsObject()
        || !request_json.HasMember("username") || !request_json["username"].IsString()
        || !request_json.HasMember("password") || !request_json["password"].IsString()) {
        request.SetResponseStatus(HttpStatus::kBadRequest);
        return {};
    }

    std::string username = request_json["username"].As<std::string>();
    std::string password = request_json["password"].As<std::string>();

    auto span = tracing::Span::CurrentSpan().CreateChild("auth_login_http");

    auto result = user_service_.CreateSession(dto::UserCredentials{std::move(username), std::move(password)});
    if (!result) {
        switch (result.error()) {
            case RefreshSessionError::kNoUserExists:
            case RefreshSessionError::kUnauthorized: {
                request.SetResponseStatus(HttpStatus::kUnauthorized);
                return {};
            }
            default: {
                LOG_DEBUG() << "CreateSession err: " << static_cast<int>(result.error());
                request.SetResponseStatus(HttpStatus::InternalServerError);
                return {};
            }
        }
    }

    auto refresh_tk_cookie = cookie_factory_.MakeRefreshTkCookie(
        std::move(result.value().refresh_tk),
        result.value().access_tk_expires_at
    );

    request.GetHttpResponse().SetCookie(refresh_tk_cookie);
    return formats::json::MakeObject("access_tk", std::move(result.value().access_tk));
}

} // namespace user_service
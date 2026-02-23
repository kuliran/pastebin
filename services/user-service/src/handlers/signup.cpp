#include "handlers/signup.hpp"
#include "services/dto/user_dto.hpp"
#include "utils/cookie.hpp"

#include <userver/formats/json.hpp>

using namespace userver;

namespace user_service {

Signup::Signup(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , user_service_(component_context.FindComponent<UserService>(UserService::kName))
{}

formats::json::Value Signup::
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

    auto span = tracing::Span::CurrentSpan().CreateChild("auth_signup_http");
    LOG_DEBUG() << "sign up";
    
    auto result = user_service_.CreateUser(UserCredentials{username, password});
    if (!result) {
        switch (result.error()) {
            case CreateUserError::kUsernameExists: {
                LOG_DEBUG() << "cannot sign up, username already exists: " << username;
                request.SetResponseStatus(HttpStatus::kConflict);
                return {};
            }
            default: {
                LOG_DEBUG() << "CreateUser err: " << static_cast<int>(result.error());
                request.SetResponseStatus(HttpStatus::InternalServerError);
                return {};
            }
        }
    }

    LOG_DEBUG() << "Sending tk: " << result.value().refresh_tk;

    auto refresh_tk_cookie = cookie::MakeRefreshTkCookie(
        std::move(result.value().refresh_tk),
        result.value().access_tk_expires_at
    );
    request.GetHttpResponse().SetCookie(refresh_tk_cookie);
    request.SetResponseStatus(HttpStatus::kCreated);
    return formats::json::MakeObject("access_tk", std::move(result.value().access_tk));
}

} // namespace user_service
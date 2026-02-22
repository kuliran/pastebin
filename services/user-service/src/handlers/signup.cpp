#include "handlers/signup.hpp"
#include "services/dto/user_dto.hpp" // IWYU pragma: keep; ADL json Serialize provider
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

    auto span = tracing::Span::CurrentSpan().CreateChild("signup_http");

    auto result = user_service_.CreateUser(UserCredentials{username, password});
    if (!result) {
        switch (result.error()) {
            case CreateUserError::kUsernameExists: {
                request.SetResponseStatus(HttpStatus::kConflict);
                return {};
            }
            default: {
                request.SetResponseStatus(HttpStatus::InternalServerError);
                return {};
            }
        }
    }

    // TODO cookie
    return formats::json::MakeObject("access_tk", std::move(result.value().access_token));
}

} // namespace user_service
#include "handlers/refresh.hpp"
#include "services/dto/user_dto.hpp"
#include "utils/cookie.hpp"

#include <userver/formats/json.hpp>

using namespace userver;

namespace user_service {

Refresh::Refresh(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , user_service_(component_context.FindComponent<UserService>(UserService::kName))
{}

formats::json::Value Refresh::
    HandleRequestJsonThrow(const HttpRequest& request, const Value&, RequestContext&)
        const {
    using userver::server::http::HttpStatus;
    using namespace user_service::dto;

    for (const auto& [name, value] : request.RequestCookies()) {
        LOG_INFO() << "Cookie: " << name << " = " << value;
    }
    if (!request.HasCookie(cookie::kRefreshTkCookieName)) {
        request.SetResponseStatus(HttpStatus::kUnauthorized);
        return {};
    }

    auto span = tracing::Span::CurrentSpan().CreateChild("auth_refresh_http");

    auto refresh_tk = request.GetCookie(cookie::kRefreshTkCookieName);
    auto result = user_service_.RefreshJwt(refresh_tk);
    if (!result) {
        switch (result.error()) {
            case RefreshJwtError::kUnauthorized: {
                request.SetResponseStatus(HttpStatus::kUnauthorized);
                return {};
            }
            default: {
                request.SetResponseStatus(HttpStatus::InternalServerError);
                return {};
            }
        }
    }

    auto refresh_tk_cookie = cookie::MakeRefreshTkCookie(
        std::move(result.value().refresh_tk),
        result.value().access_tk_expires_at
    );

    request.GetHttpResponse().SetCookie(refresh_tk_cookie);
    return formats::json::MakeObject("access_tk", std::move(result.value().access_tk));
}

} // namespace user_service
#include "handlers/refresh.hpp"

#include <userver/formats/json.hpp>

using namespace userver;

namespace user_service {

Refresh::Refresh(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , user_service_(component_context.FindComponent<AuthService>(AuthService::kName))
    , cookie_factory_(component_context.FindComponent<CookieFactory>(CookieFactory::kName))
{}

formats::json::Value Refresh::
    HandleRequestJsonThrow(const HttpRequest& request, const Value&, RequestContext&)
        const {
    using userver::server::http::HttpStatus;
    using namespace user_service::dto;

    if (!request.HasCookie(CookieFactory::kRefreshTkCookieName)) {
        request.SetResponseStatus(HttpStatus::kUnauthorized);
        return {};
    }

    auto span = tracing::Span::CurrentSpan().CreateChild("auth_refresh_http");

    auto refresh_tk = request.GetCookie(CookieFactory::kRefreshTkCookieName);
    auto result = user_service_.RefreshSession(refresh_tk);
    if (!result) {
        switch (result.error()) {
            case RefreshSessionError::kUnauthorized: {
                request.SetResponseStatus(HttpStatus::kUnauthorized);
                return {};
            }
            default: {
                LOG_DEBUG() << "RefreshSession err: " << static_cast<int>(result.error());
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
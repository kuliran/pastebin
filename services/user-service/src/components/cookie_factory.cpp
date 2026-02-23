#include "components/cookie_factory.hpp"

#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/components/component.hpp>

using namespace userver;

namespace user_service {

CookieFactory::CookieFactory(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context)
    , auth_secure_(config["secure-refresh-tk"].As<bool>(true)) {}

userver::server::http::Cookie CookieFactory::MakeRefreshTkCookie(
    std::string tk,
    std::chrono::system_clock::time_point expires_at) const
{
    userver::server::http::Cookie refresh_tk_cookie{kRefreshTkCookieName, std::move(tk)};
    refresh_tk_cookie
        .SetHttpOnly()
        .SetSameSite("strict")
        .SetPath(std::string(kRefreshJwtEndpoint))
        .SetExpires(expires_at)
    ;
    if (auth_secure_) refresh_tk_cookie.SetSecure();
    return refresh_tk_cookie;
}

userver::yaml_config::Schema CookieFactory::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
        type: object
        description: HTTP Cookie factory
        additionalProperties: false
        properties:
            secure-refresh-tk:
                type: boolean
                default: true
                description: Whether Secure property is used for auth-refresh cookie
    )");
}

}
#pragma once

#include <userver/components/component_base.hpp>
#include <userver/server/http/http_response_cookie.hpp>

namespace user_service {

class CookieFactory final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "cookie-factory";
    static constexpr std::string_view kRefreshJwtEndpoint = "/api/v2/auth/refresh";
    inline static const std::string kRefreshTkCookieName = "refresh_tk";

    CookieFactory(const userver::components::ComponentConfig& config,
                  const userver::components::ComponentContext& context);

    userver::server::http::Cookie MakeRefreshTkCookie(
        std::string tk,
        std::chrono::system_clock::time_point expires_at) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    bool auth_secure_;
};

}
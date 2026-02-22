#pragma once

#include <userver/server/http/http_response_cookie.hpp>

namespace user_service::cookie {

static constexpr std::string_view kRefreshJwtEndpoint = "/api/v2/auth/refresh";
inline static const std::string kRefreshTkCookieName = "refresh_tk";

inline userver::server::http::Cookie MakeRefreshTkCookie(std::string tk, std::chrono::system_clock::time_point expires_at) {
    userver::server::http::Cookie refresh_tk_cookie{kRefreshTkCookieName, std::move(tk)};
    refresh_tk_cookie
        .SetHttpOnly()
        .SetSecure()
        .SetSameSite("strict")
        .SetPath(std::string(kRefreshJwtEndpoint))
        .SetExpires(expires_at)
    ;
    return refresh_tk_cookie;
}

}

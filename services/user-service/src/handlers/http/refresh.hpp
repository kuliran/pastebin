#pragma once

#include "services/auth_service.hpp"
#include "components/cookie_factory.hpp"

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

namespace user_service {

class Refresh final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-auth-refresh";

    Refresh(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::formats::json::Value HandleRequestJsonThrow(const HttpRequest&, const Value&, RequestContext&)
        const override;

private:
    AuthService& auth_service_;
    CookieFactory& cookie_factory_;
};

}  // namespace user_service
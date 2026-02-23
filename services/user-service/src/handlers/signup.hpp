#pragma once

#include "services/user_service.hpp"
#include "components/cookie_factory.hpp"

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

namespace user_service {

class Signup final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-signup";

    Signup(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::formats::json::Value HandleRequestJsonThrow(const HttpRequest&, const Value&, RequestContext&)
        const override;

private:
    UserService& user_service_;
    CookieFactory& cookie_factory_;
};

}  // namespace user_service
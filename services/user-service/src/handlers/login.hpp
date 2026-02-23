#pragma once

#include "services/user_service.hpp"

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

namespace user_service {

class Login final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-login";

    Login(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::formats::json::Value HandleRequestJsonThrow(const HttpRequest&, const Value&, RequestContext&)
        const override;

private:
    UserService& user_service_;
};

}  // namespace user_service
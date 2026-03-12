// No longer used in v2, but is left here in case of a migration to CDN

#pragma once

#include <userver/components/component_base.hpp>
#include <userver/clients/http/client.hpp>

namespace write_service {

class CachePurger : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "cache-purger";

    CachePurger(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    void PurgePaste(std::string_view id) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();
private:
    userver::clients::http::Client& http_client_;
    std::string nginx_endpoint_;
};

}
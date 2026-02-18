#pragma once

#include <userver/components/component_base.hpp>
#include <userver/clients/http/client.hpp>

namespace paste_service {

class CachePurger : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "cache-purger";

    CachePurger(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    void PurgePaste(const std::string_view& id) const;
private:
    static constexpr std::string_view kNginxUrl = "http://nginx/purge/api/v1/";

    userver::clients::http::Client& http_client_;
};

}
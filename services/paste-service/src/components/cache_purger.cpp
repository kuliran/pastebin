#include "components/cache_purger.hpp"

#include <userver/components/component_context.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>

using namespace userver;

namespace paste_service {

CachePurger::CachePurger(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , http_client_(component_context.FindComponent<userver::components::HttpClient>().GetHttpClient()) {

    LOG_INFO() << "Cache purger initialized, nginx url: " << kNginxUrl;
}

void CachePurger::PurgePaste(const std::string_view& paste_id) const {
    try {
        const std::string url = std::string(kNginxUrl) + std::string(paste_id);

        LOG_INFO() << "Purging cache paste_id=" << paste_id 
                   << " url=" << url;
        
        auto response = http_client_.CreateRequest()
            .method(clients::http::HttpMethod::kGet)
            .url(url)
            .timeout(std::chrono::milliseconds(1000))
            .headers({{"X-Purge-Cache", "1"}})
            .retry(1)
            .perform();
        auto status = response->status_code();
        
        if (status == 200 || status == 404) {
            LOG_INFO() << "Cache updated paste_id=" << paste_id
                       << " status=" << status;
        } else {
            LOG_WARNING() << "Unexpected cache purge status paste_id="
                          << paste_id << " status=" << status;
        }
    } catch (const std::exception& e) {
        LOG_ERROR() << "Cache purge error paste_id=" << paste_id 
                    << " error=" << e.what();
    }
}

}
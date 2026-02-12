#pragma once

#include "components/metadata_repo.hpp"
#include "components/blob_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>
#include <userver/concurrent/background_task_storage.hpp>

namespace paste_service {

class DeletePaste final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-delete-paste";

    DeletePaste(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::formats::json::Value HandleRequestJsonThrow(const HttpRequest&, const Value&, RequestContext&)
        const override;

private:
    static constexpr std::string_view kPathArgId = "id";

    MetadataRepo& metadata_repo_;
    BlobRepo& blob_repo_;
    mutable userver::concurrent::BackgroundTaskStorage background_tasks_;
};

}  // namespace paste_service
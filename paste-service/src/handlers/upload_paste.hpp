#pragma once

#include "repo/metadata_repo.hpp"
#include "repo/blob_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_json_base.hpp>

namespace paste_service {

class UploadPaste final : public userver::server::handlers::HttpHandlerJsonBase {
public:
    static constexpr std::string_view kName = "handler-upload-paste";

    UploadPaste(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::formats::json::Value HandleRequestJsonThrow(const HttpRequest&, const Value&, RequestContext&)
        const override;

private:
    static constexpr int kIdCollisionRetries = 2;
    static constexpr int kMaxBlobSizeBytes = 1024*1024;

    MetadataRepo metadata_repo_;
    BlobRepo blob_repo_;
};

}  // namespace paste_service
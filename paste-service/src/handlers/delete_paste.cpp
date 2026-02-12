#include "handlers/delete_paste.hpp"

#include <userver/formats/json.hpp>

using namespace userver;

namespace paste_service {

DeletePaste::DeletePaste(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , metadata_repo_(config, component_context)
    , blob_repo_(config, component_context)
{}

formats::json::Value DeletePaste::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext&)
        const {
    using userver::server::http::HttpStatus;

    const auto& id = request.GetPathArg(kPathArgId);
    if (id.empty() || id.size() > 128) {
        request.SetResponseStatus(HttpStatus::kBadRequest);
        return {};
    }

    if (!request_json.IsObject() || !request_json.HasMember("delete_key")
        || !request_json["delete_key"].IsString()) {
        request.SetResponseStatus(HttpStatus::kBadRequest);
        return {};
    }

    std::string delete_key = request_json["delete_key"].As<std::string>();
    if (delete_key.empty()) {
        request.SetResponseStatus(HttpStatus::kBadRequest);
        return {};
    }

    bool existed = true;
    auto metadata_err = metadata_repo_.DeletePasteMetadata(id, delete_key);
    if (metadata_err) {
        existed = false;

        switch (*metadata_err) {
            case DeletePasteMetadataError::kNotExists:
                break;
            default: {
                request.SetResponseStatus(HttpStatus::kInternalServerError);
                return {};
            }
        }
    }

    if (existed) {
        // Background orphan blob cleanup
        background_tasks_.AsyncDetach(
            "blob-cleanup",
            [blob_repo = blob_repo_, // passing repo by value - it's a lightweight wrapper
                id = std::move(id)]() {
                try {
                    blob_repo.DeletePasteBlob(id);
                } catch (const engine::TaskCancelledException&) {
                    LOG_WARNING() << "Blob cleanup cancelled during shutdown; paste_id: " << id;
                }
            }
        );
    }

    // always tell the client the paste has been deleted for security
    request.SetResponseStatus(HttpStatus::kNoContent);
    return {};
}

}  // namespace paste_service
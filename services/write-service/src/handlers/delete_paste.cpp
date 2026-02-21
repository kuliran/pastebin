#include "handlers/delete_paste.hpp"

#include <userver/formats/json.hpp>

using namespace userver;

namespace write_service {

DeletePaste::DeletePaste(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , write_service_(component_context.FindComponent<WriteService>(WriteService::kName))
{}

formats::json::Value DeletePaste::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext&)
        const {
    using userver::server::http::HttpStatus;
    using namespace write_service::dto;

    if (!request_json.IsObject() || !request_json.HasMember("delete_key")
        || !request_json["delete_key"].IsString()) {
        request.SetResponseStatus(HttpStatus::kBadRequest);
        return {};
    }
    const auto& id = request.GetPathArg("id");
    std::string delete_key = request_json["delete_key"].As<std::string>();
    
    auto span = tracing::Span::CurrentSpan().CreateChild("delete_paste_http");
    span.AddTag("paste_id", std::string(id));

    auto result = write_service_.DeletePaste(id, delete_key);
    if (!result) {
        switch (result.error()) {
            case DeletePasteError::kInvalidId:
            case DeletePasteError::kInvalidDeleteKey: {
                request.SetResponseStatus(HttpStatus::kBadRequest);
                return {};
            }
            case DeletePasteError::kNotExists:
                break;
            default: {
                request.SetResponseStatus(HttpStatus::kInternalServerError);
                return {};
            }
        }
    }

    // always tell the client the paste has been deleted for security
    request.SetResponseStatus(HttpStatus::kNoContent);
    return {};
}

}  // namespace paste_service
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
    HandleRequestJsonThrow(const HttpRequest& request, const Value&, RequestContext& ctx)
        const {
    using userver::server::http::HttpStatus;
    using namespace write_service::dto;

    const auto& id = request.GetPathArg("id");
    const std::string& user_id = ctx.GetData<std::string>("user_id");

    auto span = tracing::Span::CurrentSpan().CreateChild("delete_paste_http");
    span.AddTag("paste_id", std::string(id));
    span.AddTag("user_id", user_id);

    auto result = write_service_.DeletePaste(id, user_id);
    if (!result) {
        switch (result.error()) {
            case DeletePasteError::kInvalidId: {
                request.SetResponseStatus(HttpStatus::kBadRequest);
                return {};
            }
            case DeletePasteError::kUnauthorized: {
                request.SetResponseStatus(HttpStatus::kForbidden);
                return {};
            }
            case DeletePasteError::kNotExists:
            case DeletePasteError::kAlreadySoftDeleted:
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

}  // namespace write_service
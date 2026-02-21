#include "handlers/get_paste.hpp"
#include "services/dto/paste_dto_json.hpp" // IWYU pragma: keep; ADL json Serialize provider

using namespace userver;

namespace read_service {

GetPaste::GetPaste(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , read_service_(component_context.FindComponent<ReadService>(ReadService::kName))
{}

formats::json::Value GetPaste::
    HandleRequestJsonThrow(const HttpRequest& request, const Value&, RequestContext&)
        const {
    using userver::server::http::HttpStatus;
    using namespace read_service::dto;

    const auto& id = request.GetPathArg("id");

    auto span = tracing::Span::CurrentSpan().CreateChild("get_paste_http");
    span.AddTag("paste_id", std::string(id));

    auto result = read_service_.GetPaste(id);
    if (!result) {
        switch (result.error()) {
            case GetPasteError::kSoftExpired:
            case GetPasteError::kNotExists: {
                request.SetResponseStatus(HttpStatus::NotFound);
                return {};
            }
            default: {
                request.SetResponseStatus(HttpStatus::InternalServerError);
                return {};
            }
        }
    }

    return formats::json::ValueBuilder(std::move(result.value())).ExtractValue();
}

}  // namespace paste_service
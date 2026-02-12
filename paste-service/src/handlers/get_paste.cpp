#include "handlers/get_paste.hpp"
#include "services/dto/paste_dto_json.hpp" // IWYU pragma: keep; ADL json Serialize provider

using namespace userver;

namespace paste_service {

GetPaste::GetPaste(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , paste_service_(component_context.FindComponent<PasteService>(PasteService::kName))
{}

formats::json::Value GetPaste::
    HandleRequestJsonThrow(const HttpRequest& request, const Value&, RequestContext&)
        const {
    using userver::server::http::HttpStatus;
    using namespace paste_service::dto;

    const auto& id = request.GetPathArg("id");

    auto span = tracing::Span::CurrentSpan().CreateChild("get_paste_http");
    span.AddTag("paste_id", std::string(id));

    auto result = paste_service_.GetPaste(id);
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
#include "handlers/get_paste_details.hpp"
#include "services/dto/paste_dto_json.hpp" // IWYU pragma: keep; ADL json Serialize provider

using namespace userver;

namespace read_service {

GetPasteDetails::GetPasteDetails(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , read_service_(component_context.FindComponent<ReadService>(ReadService::kName))
{}

formats::json::Value GetPasteDetails::
    HandleRequestJsonThrow(const HttpRequest& request, const Value&, RequestContext& ctx)
        const {
    using userver::server::http::HttpStatus;
    using namespace read_service::dto;

    const auto& id = request.GetPathArg("id");
    const std::string& user_id = ctx.GetData<std::string>("user_id");

    auto span = tracing::Span::CurrentSpan().CreateChild("get_paste_details_http");
    span.AddTag("user_id", user_id);
    span.AddTag("paste_id", std::string(id));

    auto result = read_service_.GetPasteDetails(id, user_id);
    if (!result) {
        switch (result.error()) {
            case GetPasteDetailsError::kUnauthorized: { request.SetResponseStatus(HttpStatus::kForbidden); return {}; }
            case GetPasteDetailsError::kNotExists: { request.SetResponseStatus(HttpStatus::NotFound); return {}; }
            case GetPasteDetailsError::kDbError: { request.SetResponseStatus(HttpStatus::InternalServerError); return {}; }
        }
    }

    return formats::json::ValueBuilder(std::move(result.value())).ExtractValue();
}

}  // namespace read_service
#include "handlers/get_my_pastes.hpp"
#include "services/dto/paste_dto_json.hpp" // IWYU pragma: keep; ADL json Serialize provider

using namespace userver;

namespace read_service {

GetMyPastes::GetMyPastes(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , read_service_(component_context.FindComponent<ReadService>(ReadService::kName))
{}

formats::json::Value GetMyPastes::
    HandleRequestJsonThrow(const HttpRequest& request, const Value&, RequestContext& ctx)
        const {
    using userver::server::http::HttpStatus;
    using namespace read_service::dto;

    const std::string& user_id = ctx.GetData<std::string>("user_id");

    auto span = tracing::Span::CurrentSpan().CreateChild("get_my_pastes_http");
    span.AddTag("user_id", user_id);

    auto result = read_service_.GetUserPastes(user_id);
    if (!result) {
        switch (result.error()) {
            case GetUserPastesError::kDbError: { request.SetResponseStatus(HttpStatus::InternalServerError); return {}; }
        }
    }
    return formats::json::ValueBuilder(std::move(result.value())).ExtractValue();
}

}  // namespace read_service
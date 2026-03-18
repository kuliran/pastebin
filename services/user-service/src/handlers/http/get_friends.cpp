#include "handlers/http/get_friends.hpp"

#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/json.hpp>

using namespace userver;

namespace user_service {

GetFriends::GetFriends(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , friend_service_(component_context.FindComponent<FriendService>(FriendService::kName))
{}

formats::json::Value GetFriends::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext& ctx)
        const {
    using userver::server::http::HttpStatus;
    using namespace user_service::dto;

    const std::string& user_id = ctx.GetData<std::string>("user_id");

    auto span = tracing::Span::CurrentSpan().CreateChild("get_friends_http");
    span.AddTag("user_id", user_id);

    auto result = friend_service_.GetFriends(user_id);
    if (!result) {
        switch (result.error()) {
            case GetFriendsError::kDbError: { request.SetResponseStatus(HttpStatus::InternalServerError); return {}; }
        }
    }

    return formats::json::ValueBuilder(std::move(result.value())).ExtractValue();
}

} // namespace user_service
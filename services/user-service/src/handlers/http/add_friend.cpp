#include "handlers/http/add_friend.hpp"

#include <userver/formats/serialize/common_containers.hpp>
#include <userver/formats/json.hpp>

using namespace userver;

namespace user_service {

AddFriend::AddFriend(
    const components::ComponentConfig& config,
    const components::ComponentContext& component_context
)
    : HttpHandlerJsonBase(config, component_context)
    , friend_service_(component_context.FindComponent<FriendService>(FriendService::kName))
{}

formats::json::Value AddFriend::
    HandleRequestJsonThrow(const HttpRequest& request, const Value& request_json, RequestContext& ctx)
        const {
    using userver::server::http::HttpStatus;
    using namespace user_service::dto;

    const std::string& friend_id = request.GetPathArg("user_id");
    const std::string& user_id = ctx.GetData<std::string>("user_id");

    auto span = tracing::Span::CurrentSpan().CreateChild("add_friend_http");
    span.AddTag("user_id", user_id);

    auto result = friend_service_.AddFriend(user_id, friend_id);
    if (!result) {
        switch (result.error()) {
            case AddFriendError::kInvalidInput: { request.SetResponseStatus(HttpStatus::kBadRequest); return {}; }
            case AddFriendError::kFriendLimitReached: {
                request.SetResponseStatus(HttpStatus::kConflict);
                return formats::json::MakeObject("msg", "max friend limit reached");
            }
            case AddFriendError::kDbError: { request.SetResponseStatus(HttpStatus::InternalServerError); return {}; }
        }
    }

    return {};
}

} // namespace user_service
#include "services/friend_service.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;
using namespace user_service::dto;

namespace user_service {

FriendService::FriendService(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , friend_repo_(component_context.FindComponent<FriendRepo>(FriendRepo::kName))
{}

userver::utils::expected<bool, AreFriendsError>
FriendService::AreFriends(std::string user_id, std::string friend_id) const {
    if (user_id == friend_id) return false;

    auto result = friend_repo_.AreFriends(std::move(user_id), std::move(friend_id));
    if (!result) {
        return {result.error()};
    }

    return result.value();
}

}
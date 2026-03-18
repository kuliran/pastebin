#include "services/friend_service.hpp"

#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;
using namespace user_service::dto;
using userver::utils::expected;

namespace user_service {

FriendService::FriendService(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , friend_repo_(component_context.FindComponent<FriendRepo>(FriendRepo::kName))
    , max_friends_(config["max-friends"].As<std::int32_t>())
{}

expected<dto::GetFriendsResult, dto::GetFriendsError> FriendService::GetFriends(std::string user_id) const {
    auto result = friend_repo_.GetFriends(std::move(user_id));
    if (!result) return {result.error()};

    return result.value();
}

userver::utils::expected<AddFriendResult, AddFriendError>
FriendService::AddFriend(std::string user_id, std::string friend_id) const {
    if (user_id == friend_id) return {AddFriendError::kInvalidInput};

    auto unit = friend_repo_.BeginUnit();

    auto friend_count = friend_repo_.GetFriendCount(unit, user_id);
    if (!friend_count) return {AddFriendError::kDbError};
    if (friend_count.value() >= max_friends_) return {AddFriendError::kFriendLimitReached};

    auto err = friend_repo_.AddFriend(unit, std::move(user_id), std::move(friend_id));
    if (err) {
        switch (*err) {
            case AddFriendRepoError::kAlreadyFriends: break;
            case AddFriendRepoError::kDbError: return {AddFriendError::kDbError};
        }
    }

    unit.Commit();
    return AddFriendResult{};
}

userver::utils::expected<RmFriendResult, RmFriendError>
FriendService::RmFriend(std::string user_id, std::string friend_id) const {
    auto err = friend_repo_.RmFriend(std::move(user_id), std::move(friend_id));
    if (err) return {*err};

    return RmFriendResult{};
}

expected<bool, AreFriendsError> FriendService::AreFriends(std::string user_id, std::string friend_id) const {
    if (user_id == friend_id) return false;

    auto result = friend_repo_.AreFriends(std::move(user_id), std::move(friend_id));
    if (!result) return {result.error()};

    return result.value();
}

userver::yaml_config::Schema FriendService::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<userver::components::ComponentBase>(R"(
        type: object
        description: Service that controls friend relations
        additionalProperties: false
        properties:
            max-friends:
                type: integer
                description: How many friends can a user have at most
    )");
}

}
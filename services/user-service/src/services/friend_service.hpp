#pragma once

#include "components/friend_repo.hpp"
#include "services/dto/friend_dto.hpp"

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>

namespace user_service {

class FriendService final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "friend-service";

    FriendService(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<dto::GetFriendsResult, dto::GetFriendsError> GetFriends(std::string user_id) const;
    userver::utils::expected<dto::AddFriendResult, dto::AddFriendError> AddFriend(std::string user_id, std::string friend_id) const;
    userver::utils::expected<dto::RmFriendResult, RmFriendError> RmFriend(std::string user_id, std::string friend_id) const;
    userver::utils::expected<bool, dto::IsFriendError> IsFriend(std::string user_id, std::string friend_id) const;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    FriendRepo& friend_repo_;
    std::int32_t max_friends_;
};

}
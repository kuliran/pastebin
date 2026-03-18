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

    userver::utils::expected<bool, dto::AreFriendsError>
    AreFriends(std::string user_id, std::string friend_id) const;

private:
    FriendRepo& friend_repo_;
};

}
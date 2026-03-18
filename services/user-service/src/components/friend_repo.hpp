#pragma once

#include "services/dto/friend_dto.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/expected.hpp>

namespace user_service {

class FriendRepo final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "friend-repo";

    FriendRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<bool, dto::AreFriendsError>
    AreFriends(std::string user_id, std::string friend_id) const;

private:
    static constexpr std::string_view kDefaultPgComponent = "postgres-db-1";

    userver::storages::postgres::ClusterPtr pg_cluster_;
};
    
}
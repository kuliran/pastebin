#pragma once

#include "services/dto/friend_dto.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/expected.hpp>

namespace user_service {

enum class GetFriendCountError { kDbError };
enum class AddFriendRepoError { kAlreadyFriends, kDbError };
enum class RmFriendError { kDbError };

class FriendRepo final : public userver::components::LoggableComponentBase {
public:
    using UnitOfWork = userver::storages::postgres::Transaction;

    static constexpr std::string_view kName = "friend-repo";

    FriendRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    UnitOfWork BeginUnit();

    userver::utils::expected<dto::GetFriendsResult, dto::GetFriendsError> GetFriends(std::string user_id) const;
    userver::utils::expected<int32_t, GetFriendCountError> GetFriendCount(UnitOfWork& u, std::string user_id) const;
    std::optional<AddFriendRepoError> AddFriend(UnitOfWork& u, std::string user_id, std::string friend_id) const;
    std::optional<RmFriendError> RmFriend(std::string user_id, std::string friend_id) const;
    userver::utils::expected<bool, dto::AreFriendsError> AreFriends(std::string user_id, std::string friend_id) const;

private:
    static constexpr std::string_view kDefaultPgComponent = "postgres-db-1";

    userver::storages::postgres::ClusterPtr pg_cluster_;
};
    
}
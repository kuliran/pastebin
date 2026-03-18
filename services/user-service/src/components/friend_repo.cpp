#include "components/friend_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;
using namespace user_service::dto;

namespace user_service {

FriendRepo::FriendRepo(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , pg_cluster_(component_context.FindComponent<components::Postgres>(kDefaultPgComponent).GetCluster())
{}

userver::utils::expected<bool, AreFriendsError>
FriendRepo::AreFriends(std::string user_id, std::string friend_id) const {
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,
            "SELECT 1 "
            "FROM users.friend_relations "
            "WHERE user_id = $1 AND friend_id = $2",
            std::move(user_id),
            std::move(friend_id)
        );

        return !result.IsEmpty();
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {AreFriendsError::kDbError};
    }
}



}
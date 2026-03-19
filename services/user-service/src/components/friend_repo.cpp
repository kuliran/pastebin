#include "components/friend_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;
using namespace user_service::dto;
using UnitOfWork = user_service::FriendRepo::UnitOfWork;
using userver::utils::expected;

namespace user_service {

FriendRepo::FriendRepo(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , pg_cluster_(component_context.FindComponent<components::Postgres>(kDefaultPgComponent).GetCluster())
{}

UnitOfWork FriendRepo::BeginUnit() {
    return pg_cluster_->Begin(
        storages::postgres::ClusterHostType::kMaster,
        storages::postgres::TransactionOptions{}
    );
}

expected<GetFriendsResult, GetFriendsError> FriendRepo::GetFriends(std::string user_id) const {
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,
            "SELECT friend_id "
            "FROM users.friend_relations "
            "WHERE user_id = $1",
            std::move(user_id)
        );

        return result.AsContainer<GetFriendsResult>();
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {GetFriendsError::kDbError};
    }
}

expected<int32_t, GetFriendCountError> FriendRepo::GetFriendCount(UnitOfWork& u, std::string user_id) const {
    try {
        const auto result = u.Execute(
            "WITH locked AS ( "
            "   SELECT friend_id FROM users.friend_relations WHERE user_id = $1 FOR UPDATE "
            ") "
            "SELECT count(*) FROM locked",
            std::move(user_id)
        );

        return result.AsSingleRow<int32_t>();
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {GetFriendCountError::kDbError};
    }
}

std::optional<AddFriendRepoError> FriendRepo::AddFriend(UnitOfWork& u, std::string user_id, std::string friend_id) const {
    try {
        const auto result = u.Execute(
            "INSERT INTO users.friend_relations "
            "(user_id, friend_id) VALUES ($1, $2) "
            "ON CONFLICT (user_id, friend_id) DO NOTHING",
            std::move(user_id),
            std::move(friend_id)
        );

        if (result.RowsAffected() == 0) return {AddFriendRepoError::kAlreadyFriends};

        return std::nullopt;
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {AddFriendRepoError::kDbError};
    }
}

std::optional<RmFriendError> FriendRepo::RmFriend(std::string user_id, std::string friend_id) const {
    try {
        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,
            "DELETE FROM users.friend_relations "
            "WHERE user_id = $1 AND friend_id = $2 ",
            std::move(user_id),
            std::move(friend_id)
        );

        return std::nullopt;
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {RmFriendError::kDbError};
    }
}

expected<bool, IsFriendError> FriendRepo::IsFriend(std::string user_id, std::string friend_id) const {
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
        return {IsFriendError::kDbError};
    }
}

}
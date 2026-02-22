#include "components/user_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace userver;

namespace user_service {

UserRepo::UserRepo(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , pg_cluster_(component_context.FindComponent<components::Postgres>(kDefaultPgComponent).GetCluster())
{}

userver::utils::expected<CreateUserRepoResult, CreateUserRepoError> UserRepo::CreateUserWithRefreshTk(const CreateUserParams& params) const {
    try {
        auto transaction = pg_cluster_->Begin(
            storages::postgres::ClusterHostType::kMaster,
            storages::postgres::TransactionOptions{}
        );

        const auto result = transaction.Execute(
            "INSERT INTO users.accounts "
            "(id, username, pwd_hash) "
            "VALUES ($1, $2, $3) "
            "ON CONFLICT (username) DO NOTHING",
            params.user_id,
            params.username,
            params.pwd_hash
        );

        if (result.RowsAffected() == 0) {
            transaction.Rollback();
            return {CreateUserRepoError::kUsernameExists};
        }

        const auto jwt_result = transaction.Execute(
            "INSERT INTO users.jwt_sessions "
            "(user_id, created_at, expires_at) "
            "VALUES ($1, $2, $3) "
            "RETURNING refresh_tk",
            params.user_id,
            params.jwt_access_tk_created_at,
            params.jwt_access_tk_expires_at
        );

        auto refresh_tk = boost::uuids::to_string(jwt_result.AsSingleRow<boost::uuids::uuid>());

        transaction.Commit();
        return CreateUserRepoResult{refresh_tk};
    } catch(const storages::postgres::UniqueViolation& e) {
        // In case of race condition
        LOG_WARNING() << "DB unique violation: " << e.what();
        return {CreateUserRepoError::kUsernameExists};
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {CreateUserRepoError::kDbError};
    }
}

}
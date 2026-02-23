#include "components/auth_repo.hpp"
#include "utils/crypto.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>

using namespace userver;

namespace user_service {

AuthRepo::AuthRepo(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , pg_cluster_(component_context.FindComponent<components::Postgres>(kDefaultPgComponent).GetCluster())
{}

userver::utils::expected<CreateUserRepoResult, CreateUserRepoError> AuthRepo::CreateUserWithSession(const CreateUserParams& params) const {
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
            params.jwt_refresh_tk_created_at,
            params.jwt_refresh_tk_expires_at
        );

        auto refresh_tk = boost::uuids::to_string(jwt_result.AsSingleRow<boost::uuids::uuid>());
        LOG_INFO() << "Created user user_id=" << params.user_id << " refresh_tk=" << refresh_tk
            << " pwd_hash=" << params.pwd_hash;

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

userver::utils::expected<RefreshSessionRepoResult, RefreshSessionRepoError> AuthRepo::CreateSession(
    const dto::UserCredentials& creds, std::chrono::system_clock::time_point created_at,
    std::chrono::system_clock::time_point expires_at) const {
    try {
        LOG_DEBUG() << "Trying to create new session: " << creds.username << " password=" << creds.password;

        auto transaction = pg_cluster_->Begin(
            storages::postgres::ClusterHostType::kMaster,
            storages::postgres::TransactionOptions{}
        );

        const auto result = transaction.Execute(
            "SELECT id, pwd_hash "
            "FROM users.accounts "
            "WHERE username = $1 ",
            creds.username
        );
        if (result.IsEmpty()) {
            LOG_DEBUG() << "Invalid username=" << creds.username;
            return {RefreshSessionRepoError::kNoUserExists};
        }

        auto [user_id, pwd_hash] = result.AsSingleRow<std::tuple<std::string, std::string>>(storages::postgres::kRowTag);
        if (!user_service::crypto::VerifyHash(creds.password, pwd_hash)) {
            LOG_DEBUG() << "Invalid pwd: username=" << creds.username << " password=" << creds.password;
            return {RefreshSessionRepoError::kUnauthorized};
        }

        const auto insert = transaction.Execute(
            "INSERT INTO users.jwt_sessions "
            "(user_id, created_at, expires_at) "
            "VALUES ($1, $2, $3) "
            "RETURNING refresh_tk",
            user_id,
            userver::storages::postgres::TimePointTz(created_at),
            userver::storages::postgres::TimePointTz(expires_at)
        );

        auto new_refresh_tk = boost::uuids::to_string(insert.AsSingleRow<boost::uuids::uuid>());
        LOG_DEBUG() << "New session: user_id=" << user_id << " refresh_tk=" << new_refresh_tk;

        transaction.Commit();
        return RefreshSessionRepoResult{user_id, new_refresh_tk};
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {RefreshSessionRepoError::kDbError};
    }
}

userver::utils::expected<RefreshSessionRepoResult, RefreshSessionRepoError> AuthRepo::RefreshSession(
    const std::string& refresh_tk, std::chrono::system_clock::time_point created_at,
    std::chrono::system_clock::time_point expires_at) const {
    try {
        boost::uuids::uuid cur_refresh_tk_uuid = boost::uuids::string_generator{}(refresh_tk);

        LOG_DEBUG() << "Trying to refresh session: " << refresh_tk;

        const auto result = pg_cluster_->Execute(
            storages::postgres::ClusterHostType::kMaster,
            "UPDATE users.jwt_sessions "
            "SET refresh_tk = gen_random_uuid(), created_at = $2, expires_at = $3 "
            "WHERE refresh_tk = $1 AND expires_at > NOW() "
            "RETURNING user_id, refresh_tk",
            cur_refresh_tk_uuid,
            userver::storages::postgres::TimePointTz(created_at),
            userver::storages::postgres::TimePointTz(expires_at)
        );

        if (result.RowsAffected() == 0) {
            LOG_DEBUG() << "Refresh token invalid: " << refresh_tk;
            return {RefreshSessionRepoError::kUnauthorized};
        }

        auto [user_id, new_refresh_tk_uuid] = result.AsSingleRow<std::tuple<std::string, boost::uuids::uuid>>(
            storages::postgres::kRowTag
        );
        auto new_refresh_tk = boost::uuids::to_string(std::move(new_refresh_tk_uuid));
        LOG_DEBUG() << "Refreshed token user_id=" << user_id
            << " refresh_tk=" << new_refresh_tk
            << " old_refresh_tk=" << refresh_tk;

        return RefreshSessionRepoResult{user_id, new_refresh_tk};
    } catch(const storages::postgres::Error& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {RefreshSessionRepoError::kDbError};
    } catch(const std::exception& e) {
        LOG_INFO() << "exception: " << e.what();
        return {RefreshSessionRepoError::kUnauthorized};
    }
}

}
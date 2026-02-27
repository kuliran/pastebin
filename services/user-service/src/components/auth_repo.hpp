#pragma once

#include "services/dto/auth_dto.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/utils/expected.hpp>

namespace user_service {

struct CreateUserParams {
    std::string user_id;
    std::string username;
    std::string pwd_hash;
    userver::storages::postgres::TimePointTz jwt_refresh_tk_created_at;
    userver::storages::postgres::TimePointTz jwt_refresh_tk_expires_at;
};
struct CreateUserRepoResult {
    std::string refresh_tk;
};
enum class CreateUserRepoError {
    kUsernameExists,
    kDbError,
};

struct CreateSessionRepoResult {
    std::string user_id;
    std::string refresh_tk;
};
enum class CreateSessionRepoError {
    kNoUserExists,
    kUnauthorized,
    kDbError,
};

using RefreshSessionRepoResult = CreateSessionRepoResult;
using RefreshSessionRepoError = CreateSessionRepoError;

class AuthRepo final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "user-repo";

    AuthRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<CreateUserRepoResult, CreateUserRepoError> CreateUserWithSession(const CreateUserParams&) const;

    userver::utils::expected<CreateSessionRepoResult, CreateSessionRepoError> CreateSession(
        const dto::UserCredentials& creds, std::chrono::system_clock::time_point created_at,
        std::chrono::system_clock::time_point expires_at) const;

    userver::utils::expected<RefreshSessionRepoResult, RefreshSessionRepoError> RefreshSession(
        const std::string& refresh_tk, std::chrono::system_clock::time_point created_at,
        std::chrono::system_clock::time_point expires_at) const;
private:
    static constexpr std::string_view kDefaultPgComponent = "postgres-db-1";

    userver::storages::postgres::ClusterPtr pg_cluster_;
};
    
}
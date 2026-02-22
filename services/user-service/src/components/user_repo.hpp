#pragma once

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/utils/expected.hpp>

namespace user_service {

struct CreateUserParams {
    std::string user_id;
    std::string username;
    std::string pwd_hash;
    userver::storages::postgres::TimePointTz jwt_access_tk_created_at;
    userver::storages::postgres::TimePointTz jwt_access_tk_expires_at;
};
struct CreateUserRepoResult {
    std::string refresh_token;
};
enum class CreateUserRepoError {
    kUsernameExists,
    kDbError,
};

class UserRepo final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "user-repo";
    using UserId = std::string;

    UserRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<CreateUserRepoResult, CreateUserRepoError> CreateUserWithRefreshTk(const CreateUserParams&) const;
private:
    static constexpr std::string_view kDefaultPgComponent = "postgres-db-1";

    userver::storages::postgres::ClusterPtr pg_cluster_;
};
    
}
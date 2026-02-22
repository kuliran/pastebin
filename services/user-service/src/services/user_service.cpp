#include "services/user_service.hpp"
#include "utils/crypto.hpp"

#include <userver/utils/uuid4.hpp>
#include <userver/components/component.hpp>

using namespace userver;
using namespace user_service::dto;

namespace user_service {

UserService::UserService(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , user_repo_(component_context.FindComponent<UserRepo>(UserRepo::kName))
    , jwt_issuer_(ReadFile(kPrivateKeyPath))
{}

userver::utils::expected<CreateUserResult, CreateUserError> UserService::CreateUser(const UserCredentials& creds) const {
    // TODO

    auto user_id = utils::generators::GenerateUuid();
    auto pwd_hash = user_service::crypto::HashEncode(creds.password);
    auto jwt = jwt_issuer_.Issue(jwt_wrapper::Issuer::Claims{user_id});
    const auto refresh_tk_created_at = jwt.created_at;
    const auto refresh_tk_expires_at = refresh_tk_created_at + kRefreshTkLifetime;

    auto result = user_repo_.CreateUserWithRefreshTk(CreateUserParams{
        .user_id = user_id,
        .username = creds.username,
        .pwd_hash = std::move(pwd_hash),
        .jwt_access_tk_created_at = userver::storages::postgres::TimePointTz(jwt.created_at),
        .jwt_access_tk_expires_at = userver::storages::postgres::TimePointTz(jwt.expires_at),
        .jwt_refresh_tk_created_at = userver::storages::postgres::TimePointTz(refresh_tk_created_at),
        .jwt_refresh_tk_expires_at = userver::storages::postgres::TimePointTz(refresh_tk_expires_at),
    });
    if (!result) {
        switch (result.error()) {
        case CreateUserRepoError::kUsernameExists: return {CreateUserError::kUsernameExists};
        default: return {CreateUserError::kDbError};
        }
    }

    return CreateUserResult{
        .user_id = user_id,
        .access_tk = jwt.tk,
        .refresh_tk = result.value().refresh_tk,
        .access_tk_expires_at = jwt.expires_at,
    };
}

userver::utils::expected<RefreshJwtResult, RefreshJwtError> UserService::RefreshJwt(const std::string& refresh_tk) const {
    const auto refresh_tk_created_at = std::chrono::system_clock::now();
    const auto refresh_tk_expires_at = refresh_tk_created_at + kRefreshTkLifetime;

    auto result = user_repo_.RefreshJwt(refresh_tk, refresh_tk_created_at, refresh_tk_expires_at);
    if (!result) {
        switch (result.error()) {
        case RefreshJwtRepoError::kUnauthorized: return {RefreshJwtError::kUnauthorized};
        default: return {RefreshJwtError::kDbError};
        }
    }

    auto jwt = jwt_issuer_.Issue(jwt_wrapper::Issuer::Claims{std::move(result.value().user_id)});
    return RefreshJwtResult{
        .access_tk = jwt.tk,
        .refresh_tk = result.value().refresh_tk,
        .access_tk_expires_at = jwt.expires_at,
    };
}

}
#include "services/auth_service.hpp"
#include "utils/crypto.hpp"

#include <userver/utils/uuid4.hpp>
#include <userver/components/component.hpp>

using namespace userver;
using namespace user_service::dto;

namespace user_service {

AuthService::AuthService(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , user_repo_(component_context.FindComponent<AuthRepo>(AuthRepo::kName))
    , jwt_issuer_(ReadFile(kPrivateKeyPath))
{}

userver::utils::expected<CreateUserResult, CreateUserError> AuthService::CreateUser(const UserCredentials& creds) const {
    if (creds.username.size() < 3 || creds.password.size() > 32) {
        return {CreateUserError::kInvalidUsername};
    }
    if (creds.password.size() < 6 || creds.password.size() > 48) {
        return {CreateUserError::kInvalidPassword};
    }

    auto user_id = utils::generators::GenerateUuid();
    auto pwd_hash = user_service::crypto::HashEncode(creds.password);
    const auto now = std::chrono::system_clock::now();
    const auto refresh_tk_expires_at = now + kRefreshTkLifetime;

    auto result = user_repo_.CreateUserWithSession(CreateUserParams{
        .user_id = user_id,
        .username = creds.username,
        .pwd_hash = std::move(pwd_hash),
        .jwt_refresh_tk_created_at = userver::storages::postgres::TimePointTz(now),
        .jwt_refresh_tk_expires_at = userver::storages::postgres::TimePointTz(refresh_tk_expires_at),
    });
    if (!result) {
        switch (result.error()) {
        case CreateUserRepoError::kUsernameExists: return {CreateUserError::kUsernameExists};
        default: return {CreateUserError::kDbError};
        }
    }

    auto jwt = jwt_issuer_.Issue(jwt_wrapper::Issuer::Claims{user_id}, now);

    return CreateUserResult{
        .user_id = user_id,
        .access_tk = jwt.tk,
        .refresh_tk = result.value().refresh_tk,
        .access_tk_expires_at = now,
    };
}

userver::utils::expected<dto::RefreshSessionResult, dto::RefreshSessionError>
    AuthService::CreateSession(const dto::UserCredentials& creds) const {

    const auto now = std::chrono::system_clock::now();
    const auto refresh_tk_expires_at = now + kRefreshTkLifetime;

    auto result = user_repo_.CreateSession(creds, now, refresh_tk_expires_at);
    if (!result) {
        switch (result.error()) {
        case RefreshSessionRepoError::kUnauthorized: return {RefreshSessionError::kUnauthorized};
        default: return {RefreshSessionError::kDbError};
        }
    }

    auto jwt = jwt_issuer_.Issue(jwt_wrapper::Issuer::Claims{std::move(result.value().user_id)}, now);

    return RefreshSessionResult{
        .access_tk = jwt.tk,
        .refresh_tk = result.value().refresh_tk,
        .access_tk_expires_at = jwt.expires_at,
    };
}

userver::utils::expected<RefreshSessionResult, RefreshSessionError>
    AuthService::RefreshSession(const std::string& refresh_tk) const {
    const auto now = std::chrono::system_clock::now();
    const auto refresh_tk_expires_at = now + kRefreshTkLifetime;

    auto result = user_repo_.RefreshSession(refresh_tk, now, refresh_tk_expires_at);
    if (!result) {
        switch (result.error()) {
        case RefreshSessionRepoError::kNoUserExists: return {RefreshSessionError::kNoUserExists};
        case RefreshSessionRepoError::kUnauthorized: return {RefreshSessionError::kUnauthorized};
        default: return {RefreshSessionError::kDbError};
        }
    }

    auto jwt = jwt_issuer_.Issue(jwt_wrapper::Issuer::Claims{std::move(result.value().user_id)}, now);

    return RefreshSessionResult{
        .access_tk = jwt.tk,
        .refresh_tk = result.value().refresh_tk,
        .access_tk_expires_at = jwt.expires_at,
    };
}

}
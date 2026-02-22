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

    auto result = user_repo_.CreateUserWithRefreshTk(CreateUserParams{
        .user_id = user_id,
        .username = creds.username,
        .pwd_hash = std::move(pwd_hash),
        .jwt_access_tk_created_at = userver::storages::postgres::TimePointTz(jwt.created_at),
        .jwt_access_tk_expires_at = userver::storages::postgres::TimePointTz(jwt.expires_at), 
    });
    if (!result) {
        switch (result.error()) {
        case CreateUserRepoError::kUsernameExists: return {CreateUserError::kUsernameExists};
        default: return {CreateUserError::kDbError};
        }
    }

    return CreateUserResult{
        .user_id = user_id,
        .access_token = jwt.token,
        .refresh_token = result.value().refresh_token,
        .access_tk_expires_at = jwt.expires_at,
    };
}

// jwt_wrapper::JwtToken UserService::RefreshJWT(const std::string_view& user_id, const std::string_view& refresh_tk) const {
    
// }

}
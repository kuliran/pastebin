#pragma once

#include "components/auth_repo.hpp"
#include "jwt/issuer.hpp"

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>
#include <fstream>

namespace user_service {

class AuthService final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "auth-service";

    AuthService(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<dto::CreateUserResult, dto::CreateUserError>
    CreateUser(const dto::UserCredentials& creds) const;

    userver::utils::expected<dto::CreateSessionResult, dto::CreateSessionError>
    CreateSession(const dto::UserCredentials& creds) const;
    
    userver::utils::expected<dto::RefreshSessionResult, dto::RefreshSessionError>
    RefreshSession(const std::string& refresh_tk) const;

private:
    static std::string ReadFile(std::string_view path) {
        std::ifstream f(path.data());
        if (!f) throw std::runtime_error("cannot open: " + std::string(path));
        return {std::istreambuf_iterator<char>(f), {}};
    }
private:
    static constexpr std::string_view kPrivateKeyPath = "/run/secrets/private.pem";
    static constexpr std::chrono::seconds kRefreshTkLifetime = std::chrono::hours(24*7); // 1 week

    AuthRepo& auth_repo_;
    jwt_wrapper::Issuer jwt_issuer_;
};

}
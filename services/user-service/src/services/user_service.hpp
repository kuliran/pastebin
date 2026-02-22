#pragma once

#include "components/user_repo.hpp"
#include "services/dto/user_dto.hpp"
#include "jwt/issuer.hpp"

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>
#include <fstream>

namespace user_service {

class UserService final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "user-service";

    UserService(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<dto::CreateUserResult, dto::CreateUserError> CreateUser(const dto::UserCredentials& creds) const;
    // userver::utils::expected<jwt_wrapper::JwtToken, dto::LoginError> Login(const std::string_view& user_id, const std::string_view& friend_id) const;

    // jwt_wrapper::JwtToken RefreshJWT(const std::string_view& user_id, const std::string_view& refresh_tk) const;

    // userver::utils::expected<bool, dto::GetFriendsError> AreFriends(const std::string_view& user_id, const std::string_view& friend_id) const;
    // userver::utils::expected<dto::GetFriendsResult, dto::GetFriendsError> GetFriends(const std::string_view& user_id) const;
    // userver::utils::expected<dto::AddFriendResult, dto::AddFriendError> AddFriend(const std::string_view& user_id, const std::string_view& friend_id) const;
    // userver::utils::expected<dto::RemoveFriendResult, dto::RemoveFriendError> RemoveFriend(const std::string_view& user_id, const std::string_view& friend_id) const;
private:
    static std::string ReadFile(const std::string_view& path) {
        std::ifstream f(path.data());
        if (!f) throw std::runtime_error("cannot open: " + std::string(path));
        return {std::istreambuf_iterator<char>(f), {}};
    }
private:
    static constexpr std::string_view kPrivateKeyPath = "/run/secrets/private.pem";

    UserRepo& user_repo_;
    jwt_wrapper::Issuer jwt_issuer_;
};

}
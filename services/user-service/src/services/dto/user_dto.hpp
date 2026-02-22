#pragma once

#include <string>
#include <chrono>

namespace user_service::dto {

struct UserCredentials {
    std::string username;
    std::string password;
};
struct CreateUserResult {
    std::string user_id;
    std::string access_tk;
    std::string refresh_tk;
    std::chrono::system_clock::time_point access_tk_expires_at;
};
enum class CreateUserError {
    kUsernameExists,
    kInvalidPwd,
    kDbError,
};

struct RefreshJwtResult {
    std::string access_tk;
    std::string refresh_tk;
    std::chrono::system_clock::time_point access_tk_expires_at;
};
enum class RefreshJwtError {
    kUnauthorized,
    kDbError,
};

}

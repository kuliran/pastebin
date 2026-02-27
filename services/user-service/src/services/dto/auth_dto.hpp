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
    kInvalidUsername,
    kInvalidPassword,
    kDbError,
};

struct CreateSessionResult {
    std::string user_id;
    std::string access_tk;
    std::string refresh_tk;
    std::chrono::system_clock::time_point access_tk_expires_at;
};
enum class CreateSessionError {
    kNoUserExists,
    kUnauthorized,
    kDbError,
};

struct RefreshSessionResult {
    std::string access_tk;
    std::string refresh_tk;
    std::chrono::system_clock::time_point access_tk_expires_at;
};
using RefreshSessionError = CreateSessionError;

}

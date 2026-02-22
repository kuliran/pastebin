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
    std::string access_token;
    std::string refresh_token;
    std::chrono::system_clock::time_point access_tk_expires_at;
};
enum class CreateUserError {
    kUsernameExists,
    kInvalidPwd,
    kDbError,
};

enum class CheckUserCredentialsError {
    kDbError,
};

enum class IssueJWTError {
    
};

}

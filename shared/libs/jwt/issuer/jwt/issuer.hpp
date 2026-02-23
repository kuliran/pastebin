#pragma once

#include <chrono>
#include <string>
#include <jwt-cpp/jwt.h>

namespace jwt_wrapper {

class Issuer final {
public:
    explicit Issuer(std::string private_key_pem)
        : private_key_(std::move(private_key_pem)) {}

    struct Claims {
        std::string user_id;
        std::chrono::seconds ttl = std::chrono::hours(1);
    };
    struct JwtIssueResult {
        std::string tk;
        std::chrono::system_clock::time_point expires_at;
    };

    JwtIssueResult Issue(const Claims& claims, const std::chrono::system_clock::time_point created_at) const {
        auto expires_at = created_at + claims.ttl;

        auto tk = jwt::create()
            .set_issuer("user-service")
            .set_subject(claims.user_id)
            .set_issued_at(created_at)
            .set_expires_at(expires_at)
            .sign(jwt::algorithm::rs256("", private_key_))
        ;

        return {tk, expires_at};
    }

private:
    std::string private_key_;
};

}  // namespace jwt_wrapper
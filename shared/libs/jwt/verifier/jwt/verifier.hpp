#pragma once

#include <string>
#include <stdexcept>
#include <jwt-cpp/jwt.h>

namespace jwt_wrapper {

using JwtToken = std::string;

struct VerifiedClaims {
    std::string user_id;
};

class Verifier final {
public:
    explicit Verifier(std::string public_key_pem)
        : verifier_(
            jwt::verify()
                .allow_algorithm(jwt::algorithm::rs256(std::move(public_key_pem)))
                .with_issuer("user-service")
          ) {}

    VerifiedClaims Verify(const JwtToken& token) const {
        try {
            auto decoded = jwt::decode(token);
            verifier_.verify(decoded);
            return VerifiedClaims{
                .user_id = decoded.get_subject()
            };
        } catch (const jwt::error::token_verification_exception& e) {
            throw std::runtime_error(std::string("jwt verification failed: ") + e.what());
        } catch (const std::exception& e) {
            throw std::runtime_error(std::string("jwt decode failed: ") + e.what());
        }
    }

private:
    jwt::verifier<jwt::default_clock, jwt::traits::kazuho_picojson> verifier_;
};

}  // namespace jwt_wrapper
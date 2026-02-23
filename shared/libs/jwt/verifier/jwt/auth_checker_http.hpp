#include "jwt/component.hpp"
#include "verifier.hpp"

#include <userver/server/handlers/auth/auth_checker_base.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/auth_checker_settings.hpp>
#include <userver/logging/log.hpp>

namespace jwt_wrapper {

class JwtAuthChecker final : public userver::server::handlers::auth::AuthCheckerBase {
public:
    using AuthCheckResult = userver::server::handlers::auth::AuthCheckResult;

    explicit JwtAuthChecker(Verifier verifier)
        : verifier_(std::move(verifier)) {}

    AuthCheckResult CheckAuth(
        const userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override
    {
        LOG_DEBUG() << "JWT middleware invoked";

        const auto& header = request.GetHeader("Authorization");
        if (header.empty() || !header.starts_with("Bearer ")) {
            LOG_DEBUG() << "jwt token not found";
            return AuthCheckResult{AuthCheckResult::Status::kTokenNotFound};
        }
        try {
            auto claims = verifier_.Verify(header.substr(7));
            context.SetData("user_id", std::move(claims.user_id));
        } catch (const std::exception& e) {
            LOG_DEBUG() << "jwt unauthorized attempt: " << e.what();
            return AuthCheckResult{AuthCheckResult::Status::kForbidden};
        }

        LOG_DEBUG() << "JWT middleware passed";
        return {};
    }

    bool SupportsUserAuth() const noexcept override { return true; }

private:
    Verifier verifier_;
};

class JwtCheckerFactory final 
    : public userver::server::handlers::auth::AuthCheckerFactoryBase {
public:
    static constexpr std::string_view kAuthType = "bearer";

    explicit JwtCheckerFactory(const userver::components::ComponentContext& context)
        : verifier_(context.FindComponent<JwtVerifierComponent>().GetVerifier()) {}

    userver::server::handlers::auth::AuthCheckerBasePtr MakeAuthChecker(
        const userver::server::handlers::auth::HandlerAuthConfig&) const override
    {
        return std::make_shared<JwtAuthChecker>(verifier_);
    }
    
private:
    Verifier verifier_;
};

}
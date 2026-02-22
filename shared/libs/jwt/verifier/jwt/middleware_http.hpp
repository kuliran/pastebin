#include "verifier.hpp"

#include <userver/server/middlewares/http_middleware_base.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/components/component.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/logging/log.hpp>
#include <fstream>

namespace jwt_wrapper {

class JwtMiddleware final : public userver::server::middlewares::HttpMiddlewareBase {
public:
    explicit JwtMiddleware(Verifier verifier)
        : verifier_(std::move(verifier)) {}

    void HandleRequest(userver::server::http::HttpRequest& request,
        userver::server::request::RequestContext& context) const override {
        LOG_INFO() << "@@ middleware";
        if (!skip_endpoints_.contains(request.GetRequestPath())) {
            const auto& header = request.GetHeader("Authorization");
            if (header.empty() || !header.starts_with("Bearer ")) {
                request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
                return;
            }

            try {
                auto claims = verifier_.Verify(header.substr(7));
                context.SetData("user_id", std::move(claims.user_id));
            } catch (const std::runtime_error& e) {
                LOG_INFO() << "jwt unauthorized attempt: " << e.what();
                request.SetResponseStatus(userver::server::http::HttpStatus::kUnauthorized);
                return;
            }
        }
        LOG_INFO() << "@@ next";
        Next(request, context);
    }

private:
    Verifier verifier_;
    const std::unordered_set<std::string> skip_endpoints_;
};


class JwtMiddlewareFactory final 
    : public userver::server::middlewares::HttpMiddlewareFactoryBase {
public:
    static constexpr std::string_view kName = "jwt-middleware";

    explicit JwtMiddlewareFactory(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context
    ) : HttpMiddlewareFactoryBase(config, context)
        , verifier_(LoadPublicKey(config["public-key-path"].As<std::string>()))
        , skip_endpoints_(config["skip-endpoints"].As<std::unordered_set<std::string>>()) {}

    std::unique_ptr<userver::server::middlewares::HttpMiddlewareBase>
    Create(const userver::server::handlers::HttpHandlerBase&, 
           userver::yaml_config::YamlConfig) const override {
        return std::make_unique<JwtMiddleware>(verifier_);
    }

    static userver::yaml_config::Schema GetStaticConfigSchema() {
        return userver::yaml_config::MergeSchemas<
            userver::server::middlewares::HttpMiddlewareFactoryBase
        >(R"(
            type: object
            description: JWT middleware factory
            additionalProperties: false
            properties:
                public-key-path:
                    type: string
                    description: Path to RSA public key PEM file
                skip-endpoints:
                    type: array
                    description: Endpoint paths that do not require JWT auth
                    default: []
                    items:
                        type: string
                        description: Endpoint path that does not require JWT auth
        )");
    }

private:
    static Verifier LoadPublicKey(const std::string& path) {
        std::ifstream f(path);
        return Verifier{{std::istreambuf_iterator<char>(f), {}}};
    }

    Verifier verifier_;
    const std::unordered_set<std::string> skip_endpoints_;
};

}
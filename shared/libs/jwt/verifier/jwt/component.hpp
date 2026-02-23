#pragma once

#include "verifier.hpp"

#include <userver/components/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <fstream>

namespace jwt_wrapper {

class JwtVerifierComponent final : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "jwt-verifier";

    JwtVerifierComponent(
        const userver::components::ComponentConfig& config,
        const userver::components::ComponentContext& context)
        : LoggableComponentBase(config, context)
        , verifier_(ReadFile(config["public-key-path"].As<std::string>())) {}

    const Verifier& GetVerifier() const { return verifier_; }

    static userver::yaml_config::Schema GetStaticConfigSchema() {
        return userver::yaml_config::MergeSchemas<
            userver::components::LoggableComponentBase
        >(R"(
            type: object
            description: JWT auth component
            additionalProperties: false
            properties:
                public-key-path:
                    type: string
                    description: Path to RSA public key PEM file
        )");
    }

private:
    static std::string ReadFile(std::string path) {
        std::ifstream f(std::move(path));
        return {std::istreambuf_iterator<char>(f), {}};
    }

    Verifier verifier_;
};

}
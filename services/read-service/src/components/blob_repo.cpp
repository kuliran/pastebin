#include "components/blob_repo.hpp"
#include "aws/aws_sdk_component.hpp"

#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>

#include <aws/core/auth/AWSCredentials.h>

using namespace userver;

namespace read_service {

BlobRepo::BlobRepo(const components::ComponentConfig& config, const components::ComponentContext& ctx)
    : components::LoggableComponentBase(config, ctx)
{
    // AwsSdkComponent must be initialized earlier
    ctx.FindComponent<Aws::AwsSdkComponent>();

    bucket_ = config["bucket"].As<std::string>();
    auto public_endpoint = config["public_endpoint"].As<std::string>();
    auto access_key = config["access_key"].As<std::string>();
    auto secret_key = config["secret_key"].As<std::string>();
    auto region = config["region"].As<std::string>("us-east-1");
    const bool use_https = config["use_https"].As<bool>(false);

    Aws::Client::ClientConfiguration aws_cfg;
    aws_cfg.endpointOverride = std::move(public_endpoint);
    aws_cfg.scheme = use_https ? Aws::Http::Scheme::HTTPS
                               : Aws::Http::Scheme::HTTP;
    aws_cfg.region = std::move(region);

    Aws::Auth::AWSCredentials aws_creds{std::move(access_key), std::move(secret_key)};
    aws_client_ = std::make_shared<Aws::S3::S3Client>(
        aws_creds,
        aws_cfg,
        Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
        /*useVirtualAddressing=*/false
    );
}

std::string BlobRepo::CreatePresignedGet(std::string_view paste_id, std::chrono::seconds ttl) {
    // is a pure cryptograpy operation - no thread-blocking network calls. Can be safely called in userver coroutines
    return aws_client_->GeneratePresignedUrl(
        bucket_,
        "submitted/" + std::string(paste_id),
        Aws::Http::HttpMethod::HTTP_GET,
        static_cast<long long>(ttl.count())
    );
}

userver::yaml_config::Schema BlobRepo::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<LoggableComponentBase>(R"(
        type: object
        description: Paste storage backed by S3
        additionalProperties: false
        properties:
            public_endpoint:
                type: string
                description: S3 public endpoint that the user will connect to (e.g. localhost:9000)
            bucket:
                type: string
                description: S3 bucket name
            access_key:
                type: string
                description: test
            secret_key:
                type: string
                description: test
            region:
                type: string
                description: AWS region (for signing, can be anything for MinIO)
                defaultDescription: us-east-1
            use_https:
                type: boolean
                description: Use HTTPS
                defaultDescription: false
        )"
    );
}

}
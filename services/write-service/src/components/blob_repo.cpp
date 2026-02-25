#include "components/blob_repo.hpp"
#include "components/aws_sdk_component.hpp"

#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>

#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/s3/model/DeleteObjectRequest.h>

using namespace userver;

namespace write_service {

BlobRepo::BlobRepo(const components::ComponentConfig& config, const components::ComponentContext& ctx)
    : components::LoggableComponentBase(config, ctx)
{
    // AwsSdkComponent must be initialized earlier
    ctx.FindComponent<AwsSdkComponent>();

    bucket_ = config["bucket"].As<std::string>();
    auto endpoint = config["endpoint"].As<std::string>();
    // const auto timeout = config["timeout_ms"].As<int>(500);
    // const auto retries = config["retries"].As<int>(2);
    auto access_key = config["access_key"].As<std::string>();
    auto secret_key = config["secret_key"].As<std::string>();
    auto region = config["region"].As<std::string>("us-east-1");
    const bool use_https = config["use_https"].As<bool>(false);

    Aws::Client::ClientConfiguration aws_cfg;
    aws_cfg.endpointOverride = std::move(endpoint);
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

std::string BlobRepo::CreatePresignedPut(std::string_view paste_id, std::chrono::seconds ttl) {
    // is a pure cryptograpy operation - no thread-blocking network calls. Can be safely called in userver coroutines
    return aws_client_->GeneratePresignedUrl(
        bucket_,
        "pending/" + std::string(paste_id),
        Aws::Http::HttpMethod::HTTP_PUT,
        static_cast<long long>(ttl.count())
    );
}

userver::utils::expected<BlobS3Metadata, GetPendingBlobMetadataError> BlobRepo::GetPendingBlobMetadataBlocking(std::string paste_id) {
    Aws::S3::Model::HeadObjectRequest req;
    req.SetBucket(bucket_);
    req.SetKey("pending/" + std::move(paste_id)); // key == paste_id

    auto outcome = aws_client_->HeadObject(req);
    if (!outcome.IsSuccess()) {
        const auto http_code = static_cast<int>(outcome.GetError().GetResponseCode());
        if (http_code == 404) {
            // blob is not uploaded yet
            return {GetPendingBlobMetadataError::kNotFound};
        }

        LOG_WARNING() << "GetBlobVersionId failed key=" << req.GetKey()
                      << " error=" << outcome.GetError().GetMessage();
        return {GetPendingBlobMetadataError::kDbError};
    }

    std::string version_id = outcome.GetResult().GetVersionId();
    if (version_id.empty()) {
        LOG_ERROR() << "versioning not enabled on bucket=" << bucket_;
        return {GetPendingBlobMetadataError::kDbError};
    }

    return BlobS3Metadata{
        .version_id = version_id,
        .size_bytes = outcome.GetResult().GetContentLength(),
    };
}

std::optional<SubmitBlobError> BlobRepo::SubmitBlobBlocking(std::string_view paste_id, std::string_view version_id) {
    Aws::S3::Model::CopyObjectRequest req;
    req.SetBucket(bucket_);
    req.SetCopySource(bucket_ + "/pending/" + std::string(paste_id));
    req.SetKey("submitted/" + std::string(paste_id) + "?versionId=" + std::string(version_id));

    auto copy_outcome = aws_client_->CopyObject(req);
    if (!copy_outcome.IsSuccess()) {
        const auto http_code = static_cast<int>(copy_outcome.GetError().GetResponseCode());
        if (http_code == 404) {
            return {SubmitBlobError::kNotFound};
        }
        LOG_WARNING() << "SubmitBlob CopyObject failed paste_id=" << paste_id
                      << " error=" << copy_outcome.GetError().GetMessage();
        return {SubmitBlobError::kDbError};
    }

    return std::nullopt;
}

bool BlobRepo::DeletePasteBlobBlocking(const std::string& key, const std::optional<std::string>& version_id) {
    Aws::S3::Model::DeleteObjectRequest req;
    req.SetBucket(bucket_);
    req.SetKey(key);

    if (version_id.has_value()) {
        req.SetVersionId(*version_id);
    }

    auto outcome = aws_client_->DeleteObject(req);
    if (!outcome.IsSuccess()) {
        const auto http_code = static_cast<int>(outcome.GetError().GetResponseCode());
        if (http_code == 404) {
            return true;
        }
        LOG_WARNING() << "DeletePasteBlob failed key=" << key
                      << " error=" << outcome.GetError().GetMessage();
        return false;
    }

    return true;
}

userver::yaml_config::Schema BlobRepo::GetStaticConfigSchema() {
    return userver::yaml_config::MergeSchemas<LoggableComponentBase>(R"(
        type: object
        description: Paste storage backed by S3
        additionalProperties: false
        properties:
            endpoint:
                type: string
                description: S3 endpoint (e.g. localhost:9000)
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
            timeout_ms:
                type: integer
                description: Request timeout ms
                defaultDescription: 500
            retries:
                type: integer
                description: Retry count
                defaultDescription: 2
        )"
    );
}

}
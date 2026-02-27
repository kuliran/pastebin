#pragma once

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>
#include <optional>

#include <aws/s3/S3Client.h>

namespace write_service {

enum class GetPendingBlobMetadataError {
    kNotFound,
    kDbError,
};

struct SubmitBlobResult {
    std::string version_id;
};
enum class SubmitBlobError {
    kNotFound,
    kDbError,
};

struct BlobS3Metadata {
    std::string version_id;
    long long size_bytes;
};

class BlobRepo final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "blob-repo";

    BlobRepo(const userver::components::ComponentConfig&,
                 const userver::components::ComponentContext&);

    std::string CreatePresignedPut(std::string_view paste_id, std::chrono::seconds ttl);

    // ** Blocks the whole thread, call in userver::utils::Async!
    userver::utils::expected<BlobS3Metadata, GetPendingBlobMetadataError> GetPendingBlobMetadataBlocking(std::string paste_id);

    // ** Blocks the whole thread, call in userver::utils::Async!
    userver::utils::expected<SubmitBlobResult, SubmitBlobError> SubmitBlobBlocking(std::string_view paste_id, std::string_view version_id);

    bool DeletePasteBlobBlocking(const std::string& key, const std::optional<std::string>& version_id);

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    std::shared_ptr<Aws::S3::S3Client> aws_client_;
    std::shared_ptr<Aws::S3::S3Client> aws_url_gen_client_;
    std::string bucket_;
};

}
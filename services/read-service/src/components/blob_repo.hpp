#pragma once

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>

#include <aws/s3/S3Client.h>

namespace read_service {

class BlobRepo final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "blob-repo";

    BlobRepo(const userver::components::ComponentConfig&,
                 const userver::components::ComponentContext&);

    std::string CreatePresignedGet(std::string_view paste_id, std::chrono::seconds ttl);

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    std::shared_ptr<Aws::S3::S3Client> aws_client_;
    std::string bucket_;
};

}

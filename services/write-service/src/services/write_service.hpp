#pragma once

#include "components/metadata_repo.hpp"
#include "components/blob_repo.hpp"
#include "services/dto/paste_dto.hpp"

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>
#include <userver/concurrent/background_task_storage.hpp>

namespace write_service {

class WriteService : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "write-service";

    WriteService(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<dto::CreateUploadPresignedUrlResult, dto::CreateUploadPresignedUrlError>
        CreateUploadPresignedUrl(std::string user_id, dto::UploadPasteLifetime lifetime) const;

        
    userver::utils::expected<dto::SubmitUploadResult, dto::SubmitUploadError>
        SubmitUpload(std::string paste_id, std::string_view user_id) const;

    userver::utils::expected<dto::DeletePasteResult, dto::DeletePasteError> DeletePaste(const std::string_view& presigned_url,
        const std::string_view& user_id) const;
private:
    static constexpr std::chrono::seconds kPresignedPutUrlTtl = std::chrono::seconds{5*60};
    static constexpr int kMaxBlobSizeBytes = 1024*1024; // 1MB
    static constexpr int kIdCollisionRetries = 2;

    MetadataRepo& metadata_repo_;
    BlobRepo& blob_repo_;
    mutable userver::concurrent::BackgroundTaskStorage background_tasks_;
};

}
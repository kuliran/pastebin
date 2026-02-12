#pragma once

#include "components/metadata_repo.hpp"
#include "components/blob_repo.hpp"
#include "services/dto/paste_dto.hpp"

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>
#include <userver/concurrent/background_task_storage.hpp>

namespace paste_service {

class PasteService : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "paste-service";

    PasteService(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<dto::GetPasteResult, dto::GetPasteError> GetPaste(const std::string_view& id) const;
    userver::utils::expected<dto::UploadPasteResult, dto::UploadPasteError> UploadPaste(std::string text) const; // copying text because it will be a part of the result
    userver::utils::expected<dto::DeletePasteResult, dto::DeletePasteError> DeletePaste(const std::string_view& id, const std::string_view& delete_key) const;
private:
    static constexpr int kIdCollisionRetries = 2;
    static constexpr int kMaxBlobSizeBytes = 1024*1024; // 1MB

    MetadataRepo& metadata_repo_;
    BlobRepo& blob_repo_;
    mutable userver::concurrent::BackgroundTaskStorage background_tasks_;
};

}
#pragma once

#include "components/metadata_repo.hpp"
#include "components/blob_repo.hpp"
#include "services/dto/paste_dto.hpp"

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>
#include <userver/concurrent/background_task_storage.hpp>

namespace read_service {

class ReadService : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "read-service";

    ReadService(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<dto::GetPasteResult, dto::GetPasteError> GetPaste(const std::string_view& id) const;
private:
    MetadataRepo& metadata_repo_;
    BlobRepo& blob_repo_;
};

}
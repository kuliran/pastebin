#pragma once

#include "components/metadata_repo.hpp"
#include "components/blob_repo.hpp"
#include "clients/friend_client.hpp"
#include "services/dto/paste_dto.hpp"

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>
#include <userver/concurrent/background_task_storage.hpp>

namespace read_service {

class ReadService : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "read-service";

    ReadService(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<dto::GetPasteResult, dto::GetPasteError> GetPaste(std::string_view id, std::string_view user_id) const;

    userver::utils::expected<dto::GetPasteDetailsResult, dto::GetPasteDetailsError>
    GetPasteDetails(std::string_view id, std::string_view user_id) const;

    userver::utils::expected<dto::GetUserPastesResult, dto::GetUserPastesError>
    GetUserPastes(std::string_view user_id) const;
private:
    static constexpr std::chrono::seconds kPresignedGetUrlTtl = std::chrono::seconds{5*60};

    MetadataRepo& metadata_repo_;
    BlobRepo& blob_repo_;
    FriendClient& friend_client_;
};

}
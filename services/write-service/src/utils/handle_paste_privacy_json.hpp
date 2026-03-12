#pragma once

#include "services/dto/paste_dto.hpp"
#include <userver/utils/expected.hpp>

namespace write_service {

enum class HandlePastePrivacyJsonError {
    kInvalidVisibility,
    kInvalidArrayElements,
    kTooManyArrayElements,
};

userver::utils::expected<dto::PastePrivacySettings, HandlePastePrivacyJsonError>
    HandlePastePrivacyJson(userver::formats::json::Value);

}
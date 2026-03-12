#include "utils/handle_paste_privacy_json.hpp"
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <string_view>
#include <unordered_map>

static constexpr std::string_view kVisibilityParamName = "visibility";
static constexpr std::string_view kPrivatePermsAddParamName = "private_perms_add";
static constexpr std::string_view kPrivatePermsRmParamName = "private_perms_rm";
static constexpr int kPrivatePermsAddMaxSize = 10;
static constexpr int kPrivatePermsRmMaxSize = 10;

static const std::unordered_map<std::string_view, PasteVisibility> kVisibilityMap {
    {"public", PasteVisibility::kPublic},
    {"friends", PasteVisibility::kFriends},
    {"private", PasteVisibility::kPrivate},
};

namespace write_service {

userver::utils::expected<dto::PastePrivacySettings, HandlePastePrivacyJsonError>
    HandlePastePrivacyJson(userver::formats::json::Value request_json)
{
    std::optional<PasteVisibility> visibility{std::nullopt};
    if (auto param = request_json[kVisibilityParamName]; param.IsString()) {
        auto v = param.As<std::string>();
        auto it = kVisibilityMap.find(v);
        if (it == kVisibilityMap.end()) {
            return {HandlePastePrivacyJsonError::kInvalidVisibility};
        }
        visibility = it->second;
    }

    std::optional<std::vector<std::string>> private_perms_add{std::nullopt};
    if (auto param = request_json[kPrivatePermsAddParamName]; param.IsArray()) {
        if (param.GetSize() > kPrivatePermsAddMaxSize) {
            return {HandlePastePrivacyJsonError::kTooManyArrayElements};
        }
        try {
            private_perms_add = param.As<std::vector<std::string>>();
        } catch (const userver::formats::json::TypeMismatchException&) {
            return {HandlePastePrivacyJsonError::kInvalidArrayElements};
        }
    }

    std::optional<std::vector<std::string>> private_perms_rm{std::nullopt};
    if (auto param = request_json[kPrivatePermsRmParamName]; param.IsArray()) {
        if (param.GetSize() > kPrivatePermsRmMaxSize) {
            return {HandlePastePrivacyJsonError::kTooManyArrayElements};
        }
        try {
            private_perms_rm = param.As<std::vector<std::string>>();
        } catch (const userver::formats::json::TypeMismatchException&) {
            return {HandlePastePrivacyJsonError::kInvalidArrayElements};
        }
    }

    return dto::PastePrivacySettings{
        .visibility = visibility,
        .private_perms_add = std::move(private_perms_add),
        .private_perms_rm = std::move(private_perms_rm),
    };
}

}
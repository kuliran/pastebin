#pragma once

#include "models/paste_metadata.hpp"

namespace read_service::dto {

struct GetPasteResult {
    std::string presigned_url;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    PasteVisibility visibility;
    long long size_bytes;

    GetPasteResult(PasteMetadata metadata, std::string presigned_url)
        : presigned_url(std::move(presigned_url))
        , created_at(std::move(metadata.created_at))
        , expires_at(std::move(metadata.expires_at))
        , visibility(std::move(metadata.visibility))
        , size_bytes(metadata.size_bytes) {}
};
enum class GetPasteError {
    kNotExists,
    kUnauthorized,
    kSoftExpired,
    kDbError,
};

using PastePrivatePermsUserIds = std::vector<std::string>;
struct GetPasteDetailsResult {
    PastePrivatePermsUserIds private_perms_user_ids;
    std::string owner_user_id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    PasteVisibility visibility;
    long long size_bytes;

    GetPasteDetailsResult() {}
    GetPasteDetailsResult(PasteMetadata metadata, PastePrivatePermsUserIds private_perms_user_ids)
        : private_perms_user_ids(std::move(private_perms_user_ids))
        , owner_user_id(std::move(metadata.owner_user_id))
        , created_at(metadata.created_at)
        , expires_at(metadata.expires_at)
        , visibility(metadata.visibility)
        , size_bytes(metadata.size_bytes) {}
};
enum class GetPasteDetailsError { kNotExists, kUnauthorized, kDbError };

struct UserPaste {
    std::string id;
    PasteVisibility visibility;
    std::chrono::system_clock::time_point created_at;
};
using GetUserPastesResult = std::vector<UserPaste>; 
enum class GetUserPastesError { kDbError };

}

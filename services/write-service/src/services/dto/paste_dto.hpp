#pragma once

#include "models/paste_metadata.hpp"

namespace write_service::dto {

struct CreateUploadPresignedUrlResult {
    std::string presigned_url;
    std::string paste_id;
};
enum class CreateUploadPresignedUrlError {
    kInvalidLifetimeParam,
    kUserRateLimitExceeded,
    kIdCollisionRetryExceeded,
    kDbError,
};
enum class UploadPasteLifetime {
    k1Hour,
    k1Day,
    k1Week,
    k1Month,
    k3Months,
};
inline std::optional<std::chrono::seconds> ToDuration(UploadPasteLifetime lifetime) {
    using namespace std::chrono;
    
    switch (lifetime) {
        case UploadPasteLifetime::k1Hour: return hours(1);
        case UploadPasteLifetime::k1Day: return hours(24);
        case UploadPasteLifetime::k1Week: return hours(24*7);
        case UploadPasteLifetime::k1Month: return hours(24*30);
        case UploadPasteLifetime::k3Months: return hours(24*30*3);
    }
    return std::nullopt;
}

struct SubmitUploadResult {
    SubmitUploadResult() = default;
};
enum class SubmitUploadError {
    kConflict,
    kBlobNotExists,
    kBlobTooLarge,
    kBadBlobContent,
    kDbError,
};

struct DeletePasteResult {
    DeletePasteResult() = default;
};
enum class DeletePasteError {
    kNotExists,
    kUnauthorized,
    kAlreadySoftDeleted,
    kDbError,
};

struct PatchPastePrivacyResult {
    PatchPastePrivacyResult() = default;
};
enum class PatchPastePrivacyError {
    kEmptyRequest,
    kNotExists,
    kUnauthorized,
    kDbError,
};

struct PastePrivacySettings {
    std::optional<PasteVisibility> visibility;
    std::optional<std::vector<std::string>> private_perms_add;
    std::optional<std::vector<std::string>> private_perms_rm;
};

}

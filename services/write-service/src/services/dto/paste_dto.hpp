#pragma once

#include "models/paste_metadata.hpp"
#include "models/paste_blob.hpp"

namespace write_service::dto {

struct UploadPasteResult {
    std::string id;
    std::string delete_key;

    UploadPasteResult(PasteMetadata metadata)
        : id(std::move(metadata.id))
        , delete_key(std::move(metadata.delete_key)) {}
};
enum class UploadPasteError {
    kEmptyText,
    kTextTooLarge,
    kInvalidLifetimeParam,
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


struct DeletePasteResult {
    DeletePasteResult() = default;
};
enum class DeletePasteError {
    kInvalidId,
    kInvalidDeleteKey,
    kNotExists,
    kSoftExpired,
    kDbError,
};

}

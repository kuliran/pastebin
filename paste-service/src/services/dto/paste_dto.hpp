#pragma once

#include "models/paste_metadata.hpp"
#include "models/paste_blob.hpp"

namespace paste_service::dto {

struct GetPasteResult {
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::string text;
    int size_bytes;

    GetPasteResult(PasteMetadata metadata, PasteBlob blob)
        : created_at(std::move(metadata.created_at))
        , expires_at(std::move(metadata.expires_at))
        , text(std::move(blob.text))
        , size_bytes(metadata.size_bytes) {}
};
enum class GetPasteError {
    kNotExists,
    kSoftExpired,
    kDbError,
};


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
    kIdCollisionRetryExceeded,
    kDbError,
};


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

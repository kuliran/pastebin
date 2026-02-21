#pragma once

#include "models/paste_metadata.hpp"
#include "models/paste_blob.hpp"

namespace read_service::dto {

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

}

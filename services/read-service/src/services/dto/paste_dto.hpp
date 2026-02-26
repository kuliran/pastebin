#pragma once

#include "models/paste_metadata.hpp"

namespace read_service::dto {

struct GetPasteResult {
    std::string presigned_url;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    long long size_bytes;

    GetPasteResult(PasteMetadata metadata, std::string presigned_url)
        : presigned_url(std::move(presigned_url))
        , created_at(std::move(metadata.created_at))
        , expires_at(std::move(metadata.expires_at))
        , size_bytes(metadata.size_bytes) {}
};
enum class GetPasteError {
    kNotExists,
    kUnauthorized,
    kSoftExpired,
    kDbError,
};

}

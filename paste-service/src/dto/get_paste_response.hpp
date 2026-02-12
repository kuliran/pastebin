#pragma once

#include "models/paste_blob.hpp"
#include "models/paste_metadata.hpp"

#include <userver/formats/json.hpp>

namespace paste_service::dto {

struct GetPasteResponse {
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::string text;
    int size_bytes;
};

inline GetPasteResponse MakeGetPasteResponse(
    const PasteMetadata& meta,
    const PasteBlob& blob
) {
    return {
        .created_at = meta.created_at,
        .expires_at = meta.expires_at,
        .text = blob.text,
        .size_bytes = meta.size_bytes,
    };
}

}

namespace userver::formats::serialize {

inline userver::formats::json::Value Serialize(
    const paste_service::dto::GetPasteResponse& p,
    To<userver::formats::json::Value>
) {
    userver::formats::json::ValueBuilder b;
    b["created_at"] = p.created_at;
    b["expires_at"] = p.expires_at;
    b["size_bytes"] = p.size_bytes;
    b["text"] = p.text;
    return b.ExtractValue();
}

}
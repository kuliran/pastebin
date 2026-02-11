#pragma once

#include "repo/paste_metadata.hpp"

#include <userver/formats/json.hpp>

namespace paste_service::dto {

struct UploadPasteResponse {
    std::string id;
    std::string delete_key;
};

inline UploadPasteResponse MakeUploadPasteResponse(
    const PasteMetadata& meta
) {
    return {
        .id = meta.id,
        .delete_key = meta.delete_key,
    };
}

}

namespace userver::formats::serialize {

inline userver::formats::json::Value Serialize(
    const paste_service::dto::UploadPasteResponse& p,
    To<userver::formats::json::Value>
) {
    userver::formats::json::ValueBuilder b;
    b["id"] = p.id;
    b["delete_key"] = p.delete_key;
    return b.ExtractValue();
}

}
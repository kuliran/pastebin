#pragma once

#include "services/dto/paste_dto.hpp"
#include "models/paste_metadata_json.hpp" // IWYU pragma: keep; ADL PasteVisibility json Serialize provider
#include <userver/formats/json.hpp>
#include <userver/formats/serialize/common_containers.hpp>

namespace read_service::dto {

inline userver::formats::json::Value Serialize(
    const GetPasteResult& v,
    userver::formats::serialize::To<userver::formats::json::Value>
) {
    userver::formats::json::ValueBuilder b;
    b["presigned_url"] = v.presigned_url;
    b["visibility"] = v.visibility;
    b["created_at"] = v.created_at;
    b["expires_at"] = v.expires_at;
    b["size_bytes"] = v.size_bytes;
    return b.ExtractValue();
}

inline userver::formats::json::Value Serialize(
    const GetPasteDetailsResult& v,
    userver::formats::serialize::To<userver::formats::json::Value>
) {
    userver::formats::json::ValueBuilder b;
    b["private_perms"] = v.private_perms_user_ids;
    b["visibility"] = v.visibility;
    b["created_at"] = v.created_at;
    b["expires_at"] = v.expires_at;
    b["size_bytes"] = v.size_bytes;
    return b.ExtractValue();
}

inline userver::formats::json::Value Serialize(
    const UserPaste& v,
    userver::formats::serialize::To<userver::formats::json::Value>
) {
    userver::formats::json::ValueBuilder b;
    b["id"] = v.id;
    b["visibility"] = v.visibility;
    b["created_at"] = v.created_at;
    return b.ExtractValue();
}

}

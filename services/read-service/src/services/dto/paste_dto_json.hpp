#pragma once

#include "services/dto/paste_dto.hpp"

#include <userver/formats/json.hpp>

namespace read_service::dto {

inline userver::formats::json::Value Serialize(
    const GetPasteResult& p,
    userver::formats::serialize::To<userver::formats::json::Value>
) {
    userver::formats::json::ValueBuilder b;
    b["created_at"] = p.created_at;
    b["expires_at"] = p.expires_at;
    b["size_bytes"] = p.size_bytes;
    b["text"] = p.text;
    return b.ExtractValue();
}

}

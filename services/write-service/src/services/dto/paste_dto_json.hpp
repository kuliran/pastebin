#pragma once

#include "services/dto/paste_dto.hpp"

#include <userver/formats/json.hpp>

namespace write_service::dto {

inline userver::formats::json::Value Serialize(
    const UploadPasteResult& p,
    userver::formats::serialize::To<userver::formats::json::Value>
) {
    userver::formats::json::ValueBuilder b;
    b["id"] = p.id;
    b["delete_key"] = p.delete_key;
    return b.ExtractValue();
}

}

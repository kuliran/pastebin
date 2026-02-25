#pragma once

#include "services/dto/paste_dto.hpp"

#include <userver/formats/json.hpp>

namespace write_service::dto {

inline userver::formats::json::Value Serialize(
    const CreateUploadPresignedUrlResult& p,
    userver::formats::serialize::To<userver::formats::json::Value>
) {
    userver::formats::json::ValueBuilder b;
    b["presigned_url"] = p.presigned_url;
    return b.ExtractValue();
}

}

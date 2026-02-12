#pragma once

#include <userver/formats/bson.hpp>

namespace paste_service {

struct PasteBlob {
    std::string id;
    std::string text;
};

}

namespace userver::formats::serialize {

inline userver::formats::bson::Value Serialize(
    const paste_service::PasteBlob& p,
    To<userver::formats::bson::Value>
) {
    userver::formats::bson::ValueBuilder b;
    b["_id"] = p.id;
    b["text"] = p.text;
    return b.ExtractValue();
}

}

namespace userver::formats::parse {

inline paste_service::PasteBlob Parse(
    const userver::formats::bson::Value& doc,
    To<paste_service::PasteBlob>
) {
    paste_service::PasteBlob p;
    p.id = doc["_id"].As<std::string>();
    p.text = doc["text"].As<std::string>();

    return p;
}

}
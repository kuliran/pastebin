#pragma once

#include "paste_metadata.hpp"
#include <userver/formats/json.hpp>
#include <userver/formats/json/value.hpp>

inline userver::formats::json::Value Serialize(
    const PasteVisibility& v,
    userver::formats::serialize::To<userver::formats::json::Value>
) {
    userver::formats::json::ValueBuilder b;
    switch (v) {
        case PasteVisibility::kPublic: b = "public"; break;
        case PasteVisibility::kPrivate: b = "private"; break;
        case PasteVisibility::kFriends: b = "friends"; break;
    }
    return b.ExtractValue();
}
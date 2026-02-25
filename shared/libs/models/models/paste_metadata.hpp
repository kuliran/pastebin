#pragma once

#include <string>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/enum_types.hpp>

struct PasteMetadata {
    std::string id;
    std::string owner_user_id;
    userver::storages::postgres::TimePointTz created_at;
    userver::storages::postgres::TimePointTz expires_at;
    int size_bytes;
};

enum class PasteStatus { kPending, kSubmitted, kSoftDeleted };

template <>
struct userver::storages::postgres::io::CppToUserPg<PasteStatus> : userver::storages::postgres::io::EnumMappingBase<PasteStatus> {
    static constexpr DBTypeName postgres_name = "pastes.status";
    static constexpr Enumerator enumerators[]{
        {PasteStatus::kPending, "pending"},
        {PasteStatus::kSubmitted, "submitted"},
        {PasteStatus::kSoftDeleted, "deleted"},
    };
};
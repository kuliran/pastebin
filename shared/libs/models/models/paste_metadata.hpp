#pragma once

#include <string>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/enum_types.hpp>

enum class PasteVisibility { kPublic, kFriends, kPrivate };

template <>
struct userver::storages::postgres::io::CppToUserPg<PasteVisibility> : userver::storages::postgres::io::EnumMappingBase<PasteVisibility> {
    static constexpr DBTypeName postgres_name = "pastes.visibility";
    static constexpr Enumerator enumerators[]{
        {PasteVisibility::kPublic, "public"},
        {PasteVisibility::kFriends, "friends"},
        {PasteVisibility::kPrivate, "private"},
    };
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


struct PasteMetadata {
    std::string id;
    std::string owner_user_id;
    userver::storages::postgres::TimePointTz created_at;
    userver::storages::postgres::TimePointTz expires_at;
    PasteVisibility visibility;
    int size_bytes;
};
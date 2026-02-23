#pragma once

#include <string>
#include <userver/storages/postgres/io/chrono.hpp>

struct PasteMetadata {
    std::string id;
    std::string owner_user_id;
    userver::storages::postgres::TimePointTz created_at;
    userver::storages::postgres::TimePointTz expires_at;
    int size_bytes;
};

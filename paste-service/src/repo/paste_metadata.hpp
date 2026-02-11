#pragma once

#include <string>
#include <userver/storages/postgres/io/chrono.hpp>

namespace paste_service {

struct PasteMetadata {
    std::string id;
    userver::storages::postgres::TimePointTz created_at;
    userver::storages::postgres::TimePointTz expires_at;
    std::string delete_key;
    int size_bytes;
};

}

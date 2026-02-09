#pragma once

#include <string>
#include <chrono>

namespace paste_service {

struct PasteMetadata {
    std::string id;
    std::chrono::system_clock::time_point created_at;
    std::chrono::system_clock::time_point expires_at;
    std::string delete_key;
    int size_bytes;
};

}

#pragma once

#include <string>

namespace user_service::crypto {

std::string HashEncode(std::string_view password);
bool VerifyHash(std::string_view password, std::string_view encoded);

}

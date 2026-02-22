#pragma once

#include <string>

namespace user_service::crypto {

std::string HashEncode(const std::string_view& password);
bool VerifyHash(const std::string_view& password, const std::string_view& encoded);

}

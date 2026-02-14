/*
    A 64-bit random key (YouTube's is 66-bit) encoded with base58 (Bitcoin-style)
    for human-readable and url-safe ids
*/
#pragma once

#include <string>
#include <array>

namespace paste_service {

namespace id_gen {

std::string GenId();
std::string EncodeBase58(std::array<uint8_t, 8> input);

}

}

#include "utils/id_gen.hpp"

#include <cstring>
#include <array>
#include <random>

// Bitcoin-style base58 alphabet without l, I, O, 0
static constexpr char kBase58Alphabet[] = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static std::array<uint8_t, 8> Generate64BitRandom() {
    std::array<uint8_t, 8> bytes{};
    std::mt19937_64 gen(std::random_device{}());
    uint64_t id = gen();
    std::memcpy(bytes.data(), &id, 8);
    return bytes;
}

static std::string EncodeBase58(std::array<uint8_t, 8> input) {
    std::vector<uint8_t> bytes(input.begin(), input.end());
    std::string result;

    while (!bytes.empty()) {
        uint32_t remainder = 0;
        std::vector<uint8_t> new_bytes;

        for (uint8_t byte : bytes) {
            uint32_t accumulator = (remainder << 8) | byte;
            uint8_t quotient = accumulator / 58;
            remainder = accumulator % 58;

            if (!new_bytes.empty() || quotient != 0)
                new_bytes.push_back(quotient);
        }

        result.push_back(kBase58Alphabet[remainder]);
        bytes = std::move(new_bytes);
    }

    std::reverse(result.begin(), result.end());
    return result;
}

namespace paste_service {
namespace id_gen {

std::string GenId() {
    auto bytes = Generate64BitRandom();
    return EncodeBase58(bytes);
}

}
}
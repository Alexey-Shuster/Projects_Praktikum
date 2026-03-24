#pragma once

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <random>
#include <string>

#include "../common/tagged.h"

namespace app {

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

constexpr inline static short TOKEN_LENGTH = 32;

inline static bool IsTokenValid(std::string_view token) {
    // Check if token has valid format (32 hex characters)
    if (token.length() != TOKEN_LENGTH) {
        return false;
    }

    // Validate it contains only hex characters
    if (!std::all_of(token.begin(), token.end(),
                     [](char c) { return std::isxdigit(c); })) {
        return false;
    }

    return true;
}

struct TokenGen {
    Token operator()() {
        // Generate two 64-bit random numbers
        std::uniform_int_distribution<std::uint64_t> dist;
        std::uint64_t part1 = dist(generator1_);
        std::uint64_t part2 = dist(generator2_);

        // Convert to hex strings and concatenate
        std::stringstream ss;
        ss << std::hex << std::setfill('0')
           << std::setw(16) << part1
           << std::setw(16) << part2;

        return Token{ss.str()};
    }
private:
    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    // Чтобы сгенерировать токен, получите из generator1_ и generator2_
    // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
    // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
    // чтобы сделать их подбор ещё более затруднительным
};

} // namespace app

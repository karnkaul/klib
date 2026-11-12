#pragma once
#include <string>

namespace klib {
[[nodiscard]] auto vigenere_encrypt(std::string_view key, std::string_view input) -> std::string;
[[nodiscard]] auto vigenere_decrypt(std::string_view key, std::string_view input) -> std::string;
} // namespace klib

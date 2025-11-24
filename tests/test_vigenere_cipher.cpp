#include "klib/unit_test.hpp"
#include "klib/vigenere_cipher.hpp"

namespace {
using namespace klib;

TEST(vigenere_round_trip) {
	static constexpr std::string_view key_v{"fubar"};
	static constexpr std::string_view input_v{"Some ASCII text; with symbols!"};

	auto const encrypted = vigenere_encrypt(key_v, input_v);
	auto const decrypted = vigenere_decrypt(key_v, encrypted);
	EXPECT(decrypted == input_v);
}

TEST(vigenere_json) {
	static constexpr std::string_view key_v{"@ut0g3n3[r4t3d"};
	static constexpr std::string_view input_v = R"(
{
  "a": false,
  "b": 42,
  "c": "foo",
  "d": [ 1, 3, 5, 8 ],
  "e": {
    "f": null,
    "g": "bar"
  }
}
)";

	auto const encrypted = vigenere_encrypt(key_v, input_v);
	auto const decrypted = vigenere_decrypt(key_v, encrypted);
	EXPECT(decrypted == input_v);
}
} // namespace

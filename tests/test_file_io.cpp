#include "klib/file_io.hpp"
#include "klib/unit_test/unit_test.hpp"
#include "util.hpp"

namespace {
TEST_CASE(file_io_spirv) {
	struct SpirV {
		std::vector<std::uint32_t> code{};
	};

	auto const test_dir = klib::TestDir{};

	auto const spir_v = [] {
		auto ret = SpirV{};
		for (auto i = 0; i < 10; ++i) { ret.code.push_back(std::uint32_t(i + 0xffff)); }
		return ret;
	}();

	auto const path = test_dir.to_path("test.spv").string();
	EXPECT(klib::write_to_file(std::span{spir_v.code}, path));

	auto in = SpirV{};
	EXPECT(klib::copy_file_bytes_to(in.code, path));
	EXPECT(in.code == spir_v.code);

	EXPECT(klib::write_to_file("invalid", path));
	EXPECT(!klib::copy_file_bytes_to(in.code, path));
}

TEST_CASE(file_io_string) {
	auto const test_dir = klib::TestDir{};

	static constexpr auto json_v = klib::CString{R"({
	"foo": "bar",
	"val": 42
})"};

	auto const path = test_dir.to_path("test.json").string();
	EXPECT(klib::write_to_file(json_v.as_view(), path));

	auto in = std::string{};
	EXPECT(klib::read_file_bytes_to(in, path));
	EXPECT(in == json_v);
}
} // namespace

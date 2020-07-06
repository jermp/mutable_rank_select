#include "test_common.hpp"
#include "test_node.hpp"

TEST_CASE("test avx2::node128") {
    static constexpr uint64_t max_int = 1ULL << 8;
    test_node<avx2::node128, max_int>();
}

TEST_CASE("test avx2::node64") {
    static constexpr uint64_t max_int = 1ULL << 15;
    test_node<avx2::node64, max_int>();
}

TEST_CASE("test avx2::node64") {
    static constexpr uint64_t max_int = 1ULL << 21;
    test_node<avx2::node64, max_int>();
}

TEST_CASE("test avx2::node32") {
    static constexpr uint64_t max_int = 1ULL << 27;
    test_node<avx2::node32, max_int>();
}

TEST_CASE("test avx512::node512") {
    static constexpr uint64_t max_int = 1ULL << 8;
    test_node<avx512::node512, max_int>();
}

TEST_CASE("test avx512::node256") {
    static constexpr uint64_t max_int = 1ULL << 17;
    test_node<avx512::node256, max_int>();
}

TEST_CASE("test avx512::node128") {
    static constexpr uint64_t max_int = 1ULL << 25;
    test_node<avx512::node128, max_int>();
}
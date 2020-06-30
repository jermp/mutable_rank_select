#include "test_common.hpp"
#include "test_node.hpp"

TEST_CASE("test node128") {
    static constexpr uint64_t max_int = 1ULL << 8;
    test_node<node128, max_int>();
}

TEST_CASE("test node64") {
    static constexpr uint64_t max_int = 1ULL << 15;
    test_node<node64, max_int>();
}

TEST_CASE("test node64") {
    static constexpr uint64_t max_int = 1ULL << 21;
    test_node<node64, max_int>();
}

TEST_CASE("test node32") {
    static constexpr uint64_t max_int = 1ULL << 27;
    test_node<node32, max_int>();
}
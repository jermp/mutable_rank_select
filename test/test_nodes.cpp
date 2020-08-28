#include "test_common.hpp"
#include "test_node.hpp"

TEST_CASE("test avx2::node128") {
    test_node<avx2::node128, 1ULL << 8>();
    test_node<avx2::node128, 1ULL << 9>();
}

TEST_CASE("test avx2::node64") {
    test_node<avx2::node64, 1ULL << 15>();
    test_node<avx2::node64, 1ULL << 16>();
}

TEST_CASE("test avx2::node64") {
    test_node<avx2::node64, 1ULL << 21>();
    test_node<avx2::node64, 1ULL << 22>();
}

TEST_CASE("test avx2::node32") {
    test_node<avx2::node32, 1ULL << 27>();
    test_node<avx2::node32, 1ULL << 28>();
}

TEST_CASE("test avx512::node512") {
    test_node<avx512::node512, 1ULL << 8>();
    test_node<avx512::node512, 1ULL << 9>();
}

TEST_CASE("test avx512::node256") {
    test_node<avx512::node256, 1ULL << 17>();
    test_node<avx512::node256, 1ULL << 18>();
}

TEST_CASE("test avx512::node128") {
    test_node<avx512::node128, 1ULL << 25>();
    test_node<avx512::node128, 1ULL << 26>();
}
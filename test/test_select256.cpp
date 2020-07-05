#include "test_common.hpp"
#include "test_rs256.hpp"

TEST_CASE("test NOCPU_SDSL") {
    test_select<select_modes::NOCPU_SDSL>();
}

TEST_CASE("test NOCPU_BROADWORD") {
    test_select<select_modes::NOCPU_BROADWORD>();
}

TEST_CASE("test BMI1_TZCNT_SDSL") {
    test_select<select_modes::BMI1_TZCNT_SDSL>();
}

TEST_CASE("test SSE4_2_POPCNT_BROADWORD") {
    test_select<select_modes::SSE4_2_POPCNT_BROADWORD>();
}

TEST_CASE("test BMI2_PDEP_TZCNT") {
    test_select<select_modes::BMI2_PDEP_TZCNT>();
}

TEST_CASE("test AVX2_POPCNT") {
    test_select<select_modes::AVX2_POPCNT>();
}

TEST_CASE("test AVX2_POPCNT_EX") {
    test_select<select_modes::AVX2_POPCNT_EX>();
}

#include "test_common.hpp"
#include "test_rs256.hpp"

TEST_CASE("test NOCPU") {
    test_rank<rank_modes::NOCPU>();
}

TEST_CASE("test SSE4_2_POPCNT") {
    test_rank<rank_modes::SSE4_2_POPCNT>();
}

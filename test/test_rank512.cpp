#include "test_common.hpp"
#include "test_rank_select_algorithms.hpp"

TEST_CASE("test broadword_loop") {
    test_rank_512<rank_modes::broadword_loop>();
}
TEST_CASE("test broadword_unrolled") {
    test_rank_512<rank_modes::broadword_unrolled>();
}
#ifdef __SSE4_2__
TEST_CASE("test builtin_loop") {
    test_rank_512<rank_modes::builtin_loop>();
}
TEST_CASE("test builtin_unrolled") {
    test_rank_512<rank_modes::builtin_unrolled>();
}
#endif
#ifdef __AVX2__
TEST_CASE("test avx2_unrolled") {
    test_rank_512<rank_modes::avx2_unrolled>();
}
#endif
#ifdef __AVX512VL__
TEST_CASE("test avx2_parallel") {
    test_rank_512<rank_modes::avx2_parallel>();
}
#endif

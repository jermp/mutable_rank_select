#include "test_common.hpp"
#include "test_rank_select_algorithms.hpp"

TEST_CASE("test broadword_loop_sdsl") {
    test_select_256<select_modes::broadword_loop_sdsl>();
}
TEST_CASE("test broadword_loop_succinct") {
    test_select_256<select_modes::broadword_loop_succinct>();
}
#ifdef __SSE4_2__
TEST_CASE("test builtin_loop_sdsl") {
    test_select_256<select_modes::builtin_loop_sdsl>();
}
TEST_CASE("test builtin_loop_succinct") {
    test_select_256<select_modes::builtin_loop_succinct>();
}
#endif
#ifdef __BMI2__
TEST_CASE("test builtin_loop_pdep") {
    test_select_256<select_modes::builtin_loop_pdep>();
}
#endif
#ifdef __AVX512VL__
TEST_CASE("test builtin_avx512_pdep") {
    test_select_256<select_modes::builtin_avx512_pdep>();
}
TEST_CASE("test avx2_avx512_pdep") {
    test_select_256<select_modes::avx2_avx512_pdep>();
}
#endif
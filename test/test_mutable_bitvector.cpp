#include "test_common.hpp"
#include "test_mutable_bitvector.hpp"

TEST_CASE("test ") {
    static constexpr double density = 0.3;
    static constexpr uint64_t num_bits = 32768;
    static constexpr uint64_t height =
        avx2::segment_tree<1>::height((num_bits + 255) / 256);
    typedef mutable_bitvector<avx2::segment_tree<height>,
                              rank_modes::SSE4_2_POPCNT,
                              select_modes::BMI2_PDEP_TZCNT>
        vector_type;
    test_mutable_bitvector<vector_type>(num_bits, density);
}
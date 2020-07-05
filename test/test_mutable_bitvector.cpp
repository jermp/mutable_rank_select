#include "test_common.hpp"
#include "test_mutable_bitvector.hpp"

template <int i, template <uint32_t> typename Tree, rank_modes RankMode,
          select_modes SelectMode>
struct test {
    static void run() {
        const uint64_t num_bits = 1ULL << logs2[i];
        static constexpr uint64_t height =
            Tree<1>::height((num_bits + 255) / 256);
        typedef mutable_bitvector<Tree<height>, RankMode, SelectMode>
            vector_type;
        static constexpr double density = 0.3;
        test_mutable_bitvector<vector_type>(num_bits, density);
        test<i + 1, Tree, RankMode, SelectMode>::run();
    }
};

template <template <uint32_t> typename Tree, rank_modes RankMode,
          select_modes SelectMode>
struct test<sizeof(logs2) / sizeof(logs2[0]), Tree, RankMode, SelectMode> {
    static void run() {}
};

TEST_CASE("test mutable_bitvector_avx2") {
    test<0, avx2::segment_tree, rank_modes::SSE4_2_POPCNT,
         select_modes::BMI2_PDEP_TZCNT>::run();
}

TEST_CASE("test mutable_bitvector_avx512") {
    test<0, avx512::segment_tree, rank_modes::SSE4_2_POPCNT,
         select_modes::BMI2_PDEP_TZCNT>::run();
}
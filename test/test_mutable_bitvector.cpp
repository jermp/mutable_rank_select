#include "test_common.hpp"
#include "test_mutable_bitvector.hpp"

template <int i, template <uint32_t> typename PrefixSums, rank_modes RankMode,
          select_modes SelectMode>
struct test {
    static void run() {
        const uint64_t num_bits = 1ULL << logs2[i];
        static constexpr uint64_t height =
            PrefixSums<1>::height((num_bits + 255) / 256);
        typedef mutable_bitvector<PrefixSums<height>, RankMode, SelectMode>
            vector_type;
        static constexpr double density = 0.3;
        test_mutable_bitvector<vector_type>(num_bits, density);
        test<i + 1, PrefixSums, RankMode, SelectMode>::run();
    }
};

template <template <uint32_t> typename PrefixSums, rank_modes RankMode,
          select_modes SelectMode>
struct test<sizeof(logs2) / sizeof(logs2[0]), PrefixSums, RankMode,
            SelectMode> {
    static void run() {}
};

TEST_CASE("test avx2_mutable_bitvector") {
    test<0, avx2::segment_tree, rank_modes::builtin_unrolled,
         select_modes::builtin_loop_pdep>::run();
}

TEST_CASE("test avx512_mutable_bitvector") {
    test<0, avx512::segment_tree, rank_modes::builtin_unrolled,
         select_modes::builtin_loop_pdep>::run();
}
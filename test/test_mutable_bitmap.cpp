#include "test_common.hpp"
#include "test_mutable_bitmap.hpp"

template <int i, template <uint32_t> typename SearchablePrefixSums,
          typename BlockTypeTraits>
struct test {
    static void run() {
        const uint64_t num_bits = 1ULL << logs2[i];
        static constexpr uint64_t height = SearchablePrefixSums<1>::height(
            (num_bits + BlockTypeTraits::block_size - 1) /
            BlockTypeTraits::block_size);
        typedef mutable_bitmap<SearchablePrefixSums<height>, BlockTypeTraits>
            mutable_bitmap_type;
        static constexpr double density = 0.3;
        test_mutable_bitmap<mutable_bitmap_type>(num_bits, density);
        test<i + 1, SearchablePrefixSums, BlockTypeTraits>::run();
    }
};

template <template <uint32_t> typename SearchablePrefixSums,
          typename BlockTypeTraits>
struct test<sizeof(logs2) / sizeof(logs2[0]), SearchablePrefixSums,
            BlockTypeTraits> {
    static void run() {}
};

TEST_CASE("test avx2_mutable_bitmap_64") {
    test<0, avx2::segment_tree, block64_type_default>::run();
}

TEST_CASE("test avx512_mutable_bitmap_64") {
    test<0, avx512::segment_tree, block64_type_default>::run();
}

TEST_CASE("test avx2_mutable_bitmap_256") {
    test<0, avx2::segment_tree, block256_type_a>::run();
}

TEST_CASE("test avx512_mutable_bitmap_256") {
    test<0, avx512::segment_tree, block256_type_a>::run();
}

TEST_CASE("test avx2_mutable_bitmap_512") {
    test<0, avx2::segment_tree, block512_type_a>::run();
}

TEST_CASE("test avx512_mutable_bitmap_512") {
    test<0, avx512::segment_tree, block512_type_a>::run();
}
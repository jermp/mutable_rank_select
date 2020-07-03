#include "test_common.hpp"
#include "test_tree.hpp"

template <int i, template <uint32_t> typename Tree>
struct test {
    static void run() {
        const uint32_t n = sizes[i];
        const uint32_t height = Tree<1>::height(n);
        test_tree<Tree<height>>(n);
        test<i + 1, Tree>::run();
    }
};

template <template <uint32_t> typename Tree>
struct test<sizeof(sizes) / sizeof(sizes[0]), Tree> {
    static void run() {}
};

TEST_CASE("test avx2::segment_tree") {
    test<0, avx2::segment_tree>::run();
}

TEST_CASE("test avx512::segment_tree") {
    test<0, avx512::segment_tree>::run();
}
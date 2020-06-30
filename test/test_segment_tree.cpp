#include "test_common.hpp"
#include "test_tree.hpp"

template <int i, template <uint32_t> typename Tree>
struct test {
    static void run() {
        const uint64_t n = sizes[i];
        const uint32_t height = Tree<1>::height(n);
        typedef typename Tree<height>::tree_type tree_type;
        test_tree<tree_type>(n);
        test<i + 1, Tree>::run();
    }
};

template <template <uint32_t> typename Tree>
struct test<sizeof(sizes) / sizeof(sizes[0]), Tree> {
    static void run() {}
};

TEST_CASE("test segment_tree") {
    test<0, avx2::segment_tree>::run();
}
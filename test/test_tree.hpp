#pragma once

namespace mrs::testing {

template <typename Tree>
void test_tree(size_t n) {
    essentials::uniform_int_rng<uint16_t> distr(0, 256,
                                                essentials::get_random_seed());
    std::cout << "== testing " << Tree::name() << " with " << n
              << " leaves ==" << std::endl;
    std::vector<uint16_t> A(n);
    std::generate(A.begin(), A.end(), [&] { return distr.gen(); });
    Tree tree;
    essentials::logger("building tree...");
    tree.build(A.data(), n);

    {
        essentials::logger("testing sum queries...");
        static constexpr uint32_t sum_queries = 5000;
        uint32_t step = n / sum_queries;
        if (step == 0) step = 1;
        uint64_t expected = 0;
        uint32_t k = 0;
        for (uint32_t i = 0; i < n; i += step) {
            uint64_t got = tree.sum(i);
            for (; k != i + 1; ++k) expected += A[k];
            REQUIRE_MESSAGE(got == expected, "got sum(" << i << ") = " << got
                                                        << " but expected "
                                                        << expected);
        }
    }

    auto update = [&](int8_t delta) {
        bool sign = delta >> 7;
        essentials::logger("testing update(" + std::to_string(delta) + ")...");
        static constexpr uint32_t update_queries = 5000;
        uint32_t step = n / update_queries;
        if (step == 0) step = 1;
        for (uint32_t run = 0; run != 100; ++run) {
            uint64_t expected = 0;
            uint32_t k = 0;
            for (uint32_t i = 0; i < n; i += step) {
                tree.update(i, sign);
                uint64_t got = tree.sum(i);
                A[i] += delta;
                for (; k != i + 1; ++k) expected += A[k];
                REQUIRE_MESSAGE(got == expected,
                                "error during run "
                                    << run << ": got sum(" << i << ") = " << got
                                    << " but expected " << expected);
            }
        }
    };

    {
        essentials::logger("testing search queries...");
        uint64_t max_sum = tree.sum(n - 1);
        essentials::uniform_int_rng<uint64_t> distr(
            0, max_sum - 1, essentials::get_random_seed());
        for (uint32_t i = 0; i != 1000; ++i) {
            uint64_t x = distr.gen();
            uint64_t expected_position = 0;
            uint64_t expected_sum = 0;
            for (; expected_position != n; ++expected_position) {
                if (expected_sum + A[expected_position] > x) break;
                expected_sum += A[expected_position];
            }
            auto got = tree.search(x);
            bool good = (got.position == expected_position) and
                        (got.sum == expected_sum);
            REQUIRE_MESSAGE(good, "error during SEARCH: got search("
                                      << x << ") = (" << got.position << ","
                                      << got.sum << ")"
                                      << " but expected (" << expected_position
                                      << "," << expected_sum << ")");
        }
    }

    update(+1);
    update(-1);

    std::cout << "\teverything's good" << std::endl;
}

}  // namespace mrs::testing
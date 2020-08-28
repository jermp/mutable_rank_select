#pragma once

namespace dyrs::testing {

template <typename Node, uint64_t max_int>
void test_node() {
    std::cout << "# " << Node::name() << " # fanout " << Node::fanout
              << " # max_int " << max_int << std::endl;

    typedef typename Node::key_type uint_type;
    std::vector<uint_type> A(Node::fanout);
    {
        essentials::uniform_int_rng<uint_type> distr(
            0, max_int, essentials::get_random_seed());
        std::generate(A.begin(), A.end(), [&] { return distr.gen(); });
    }

    std::vector<uint8_t> out(Node::bytes);
    Node::build(A.data(), out.data());
    Node node(out.data());

    {
        essentials::logger("testing sum queries...");
        uint64_t expected = 0;
        for (uint32_t i = 0; i != Node::fanout; ++i) {
            uint64_t got = node.sum(i);
            expected += A[i];
            REQUIRE_MESSAGE(got == expected, "error during SUM: got sum("
                                                 << i << ") = " << got
                                                 << " but expected "
                                                 << expected);
        }
    }

    auto update = [&](int8_t delta) {
        bool sign = delta >> 7;
        essentials::logger("testing update(" + std::to_string(delta) + ")...");
        for (uint64_t run = 0; run != 100; ++run) {
            uint64_t expected = 0;
            for (uint64_t i = 0; i != Node::fanout; ++i) {
                node.update(i, sign);
                uint64_t got = node.sum(i);
                A[i] += delta;
                expected += A[i];
                REQUIRE_MESSAGE(got == expected,
                                "error during UPDATE("
                                    << int(delta) << ") at run " << run
                                    << ": got sum(" << i << ") = " << got
                                    << " but expected " << expected);
            }
        }
    };

    {
        essentials::logger("testing search queries...");
        uint64_t max_sum = node.sum(Node::fanout - 1);
        essentials::uniform_int_rng<uint_type> distr(
            0, max_sum - 1, essentials::get_random_seed());
        for (uint32_t i = 0; i != 10000; ++i) {
            uint64_t x = distr.gen();
            uint64_t expected_position = 0;
            uint64_t expected_sum = 0;
            for (; expected_position != Node::fanout; ++expected_position) {
                if (expected_sum + A[expected_position] > x) break;
                expected_sum += A[expected_position];
            }
            auto got = node.search(x);
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

}  // namespace dyrs::testing
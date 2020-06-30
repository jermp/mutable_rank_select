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
        for (uint64_t run = 0; run != 100; ++run) {
            uint64_t expected = 0;
            for (uint64_t i = 0; i != Node::fanout; ++i) {
                node.update(i, delta);
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
        uint64_t max_sum = node.sum(Node::fanout - 1);
        essentials::uniform_int_rng<uint_type> distr(
            0, max_sum - 1, essentials::get_random_seed());
        for (uint32_t i = 0; i != 10000; ++i) {
            uint64_t x = distr.gen();
            uint64_t expected = 0;
            for (uint64_t sum = 0; expected != Node::fanout; ++expected) {
                sum += A[expected];
                if (sum > x) break;
            }
            uint64_t got = node.search(x);
            REQUIRE_MESSAGE(got == expected, "error during SEARCH: got search("
                                                 << x << ") = " << got
                                                 << " but expected "
                                                 << expected);
        }
    }

    update(+1);
    update(-1);

    std::cout << "\teverything's good" << std::endl;
}

}  // namespace dyrs::testing
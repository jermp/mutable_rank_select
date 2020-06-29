#pragma once

namespace dyrs::testing {

template <typename Node, uint64_t max_int>
void test_node() {
    typedef typename Node::key_type uint_type;
    essentials::uniform_int_rng<uint_type> distr(0, max_int,
                                                 essentials::get_random_seed());
    std::cout << "# " << Node::name() << " # fanout " << Node::fanout
              << " # max_int " << max_int << std::endl;
    std::vector<uint_type> A(Node::fanout);
    std::vector<uint8_t> out(Node::bytes);
    std::generate(A.begin(), A.end(), [&] { return distr.gen(); });
    Node::build(A.data(), out.data());
    Node node(out.data());

    {
        uint64_t expected = 0;
        for (uint32_t i = 0; i != Node::fanout; ++i) {
            uint64_t got = node.sum(i);
            expected += A[i];
            REQUIRE_MESSAGE(got == expected, "got sum(" << i << ") = " << got
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
                                "error during run "
                                    << run << ": got sum(" << i << ") = " << got
                                    << " but expected " << expected);
            }
        }
    };

    update(+1);
    update(-1);
    std::cout << "\teverything's good" << std::endl;
}

}  // namespace dyrs::testing
#pragma once

#include "rs256/algorithms.hpp"

namespace dyrs::testing {

uint64_t naive_rank_u256(uint64_t const* x, uint64_t i) {
    assert(i < 256);
    uint64_t ones = 0;
    uint64_t pos = 0;
    for (uint64_t w = 0; w != 4; ++w) {
        uint64_t word = x[w];
        for (uint64_t bit = 0; bit != 64; ++bit) {
            ones += word & 1;
            if (pos == i) goto EXIT;
            word >>= 1;
            ++pos;
        }
    }
EXIT:
    assert(ones <= 256);
    return ones;
}

uint64_t naive_select_u256(uint64_t const* x, uint64_t i) {
    assert(i < naive_rank_u256(x, 255));
    uint64_t ones = 0;
    uint64_t pos = 0;
    i += 1;
    for (uint64_t w = 0; w != 4; ++w) {
        uint64_t word = x[w];
        for (uint64_t bit = 0; bit != 64; ++bit) {
            ones += word & 1;
            if (ones == i) goto EXIT;
            word >>= 1;
            ++pos;
        }
    }
EXIT:
    assert(pos < 256);
    return pos;
}

template <rank_modes Mode>
void test_rank() {
    std::cout << "----- Algorithm: " << print_rank_mode(Mode) << " -----"
              << std::endl;
    essentials::uniform_int_rng<uint64_t> distr(0, -1,
                                                essentials::get_random_seed());
    std::vector<uint64_t> bits{distr.gen(), distr.gen(), distr.gen(),
                               distr.gen()};
    for (uint32_t i = 0; i != 256; ++i) {
        uint64_t got = rank_u256<Mode>(bits.data(), i);
        uint64_t expected = naive_rank_u256(bits.data(), i);
        REQUIRE_MESSAGE(got == expected, "error got rank(" << i << ") = " << got
                                                           << " but expected "
                                                           << expected);
    }
    std::cout << "\teverything's good" << std::endl;
}

template <select_modes Mode>
void test_select() {
    std::cout << "----- Algorithm: " << print_select_mode(Mode) << " -----"
              << std::endl;
    essentials::uniform_int_rng<uint64_t> distr(0, -1,
                                                essentials::get_random_seed());
    std::vector<uint64_t> bits{distr.gen(), distr.gen(), distr.gen(),
                               distr.gen()};
    uint64_t total_ones = naive_rank_u256(bits.data(), 255);
    for (uint32_t i = 0; i != total_ones; ++i) {
        uint64_t got = select_u256<Mode>(bits.data(), i);
        uint64_t expected = naive_select_u256(bits.data(), i);
        REQUIRE_MESSAGE(got == expected, "error got select("
                                             << i << ") = " << got
                                             << " but expected " << expected);
    }
    std::cout << "\teverything's good" << std::endl;
}

}  // namespace dyrs::testing
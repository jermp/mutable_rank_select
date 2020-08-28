#pragma once

#include "rank_select_algorithms/rank.hpp"
#include "rank_select_algorithms/select.hpp"

namespace dyrs::testing {

template <uint64_t Words>
uint64_t naive_rank_for_words(uint64_t const* x, uint64_t i) {
    assert(i < 64 * Words);
    uint64_t ones = 0;
    uint64_t pos = 0;
    for (uint64_t w = 0; w != Words; ++w) {
        uint64_t word = x[w];
        for (uint64_t bit = 0; bit != 64; ++bit) {
            ones += word & 1;
            if (pos == i) goto EXIT;
            word >>= 1;
            ++pos;
        }
    }
EXIT:
    assert(ones <= 64 * Words);
    return ones;
}

template <uint64_t Words>
uint64_t naive_select_for_words(uint64_t const* x, uint64_t i) {
    assert(i < naive_rank_for_words<Words>(x, 64 * Words - 1));
    uint64_t ones = 0;
    uint64_t pos = 0;
    i += 1;
    for (uint64_t w = 0; w != Words; ++w) {
        uint64_t word = x[w];
        for (uint64_t bit = 0; bit != 64; ++bit) {
            ones += word & 1;
            if (ones == i) goto EXIT;
            word >>= 1;
            ++pos;
        }
    }
EXIT:
    assert(pos < 64 * Words);
    return pos;
}

template <rank_modes Mode>
void test_rank_256() {
    std::cout << "----- Algorithm: " << print_rank_mode(Mode) << " -----"
              << std::endl;
    splitmix64 distr(essentials::get_random_seed());
    std::vector<uint64_t> bits{distr.next(), distr.next(), distr.next(),
                               distr.next()};
    for (uint32_t i = 0; i != 256; ++i) {
        uint64_t got = rank_u256<Mode>(bits.data(), i);
        uint64_t expected = naive_rank_for_words<4>(bits.data(), i);
        REQUIRE_MESSAGE(got == expected, "error got rank(" << i << ") = " << got
                                                           << " but expected "
                                                           << expected);
    }
    std::cout << "\teverything's good" << std::endl;
}

template <rank_modes Mode>
void test_rank_512() {
    std::cout << "----- Algorithm: " << print_rank_mode(Mode) << " -----"
              << std::endl;
    splitmix64 distr(essentials::get_random_seed());
    std::vector<uint64_t> bits{distr.next(), distr.next(), distr.next(),
                               distr.next(), distr.next(), distr.next(),
                               distr.next(), distr.next()};
    for (uint32_t i = 0; i != 512; ++i) {
        uint64_t got = rank_u512<Mode>(bits.data(), i);
        uint64_t expected = naive_rank_for_words<8>(bits.data(), i);
        REQUIRE_MESSAGE(got == expected, "error got rank(" << i << ") = " << got
                                                           << " but expected "
                                                           << expected);
    }
    std::cout << "\teverything's good" << std::endl;
}

template <select_modes Mode>
void test_select_256() {
    std::cout << "----- Algorithm: " << print_select_mode(Mode) << " -----"
              << std::endl;
    splitmix64 distr(essentials::get_random_seed());
    std::vector<uint64_t> bits{distr.next(), distr.next(), distr.next(),
                               distr.next()};
    uint64_t total_ones = naive_rank_for_words<4>(bits.data(), 255);
    for (uint32_t i = 0; i != total_ones; ++i) {
        uint64_t got = select_u256<Mode>(bits.data(), i);
        uint64_t expected = naive_select_for_words<4>(bits.data(), i);
        REQUIRE_MESSAGE(got == expected, "error got select("
                                             << i << ") = " << got
                                             << " but expected " << expected);
    }
    std::cout << "\teverything's good" << std::endl;
}

template <select_modes Mode>
void test_select_512() {
    std::cout << "----- Algorithm: " << print_select_mode(Mode) << " -----"
              << std::endl;
    splitmix64 distr(essentials::get_random_seed());
    std::vector<uint64_t> bits{distr.next(), distr.next(), distr.next(),
                               distr.next(), distr.next(), distr.next(),
                               distr.next(), distr.next()};
    uint64_t total_ones = naive_rank_for_words<8>(bits.data(), 511);
    for (uint32_t i = 0; i != total_ones; ++i) {
        uint64_t got = select_u512<Mode>(bits.data(), i);
        uint64_t expected = naive_select_for_words<8>(bits.data(), i);
        REQUIRE_MESSAGE(got == expected, "error got select("
                                             << i << ") = " << got
                                             << " but expected " << expected);
    }
    std::cout << "\teverything's good" << std::endl;
}

}  // namespace dyrs::testing
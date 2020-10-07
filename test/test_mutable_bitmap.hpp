#pragma once

namespace mrs::testing {

template <typename MutableBitmap>
void test_mutable_bitmap(uint64_t num_bits, double density) {
    assert(num_bits >= 256);
    std::cout << "== testing " << MutableBitmap::name() << " with " << num_bits
              << " bits ==" << std::endl;

    MutableBitmap bitmap;
    uint64_t num_ones = 0;
    {
        std::vector<uint64_t> bits((num_bits + 63) / 64);
        num_ones = create_random_bits(bits, UINT64_MAX * density,
                                      essentials::get_random_seed());
        bitmap.build(bits.data(), bits.size());
    }

    auto rank = [&]() {
        essentials::logger("testing rank queries...");
        auto const& bits = bitmap.bits();
        uint64_t bits_in_word = 64;
        uint64_t word_index = 0;
        uint64_t word = bits[word_index];
        uint64_t pos = 0;
        uint64_t expected = 0;
        for (uint64_t i = 0; i != num_bits; ++i) {
            while (true) {
                expected += word & 1;
                word >>= 1;
                --bits_in_word;
                if (bits_in_word == 0) {
                    if (++word_index == bits.size()) break;
                    word = bits[word_index];
                    bits_in_word = 64;
                }
                if (pos++ == i) break;
            }
            uint64_t got = bitmap.rank(i);
            REQUIRE_MESSAGE(got == expected,
                            "error got rank(" << i << ") = " << got
                                              << " but expected " << expected);
        }
    };

    auto select = [&]() {
        essentials::logger("testing select queries...");
        auto const& bits = bitmap.bits();
        uint64_t bits_in_word = 64;
        uint64_t word_index = 0;
        uint64_t word = bits[word_index];
        uint64_t expected = -1;
        uint64_t ones = 0;
        for (uint64_t i = 0; i != num_ones; ++i) {
            while (true) {
                ones += word & 1;
                word >>= 1;
                ++expected;
                --bits_in_word;
                if (bits_in_word == 0) {
                    if (++word_index == bits.size()) break;
                    word = bits[word_index];
                    bits_in_word = 64;
                }
                if (ones == i + 1) break;
            }
            uint64_t got = bitmap.select(i);
            REQUIRE_MESSAGE(got == expected, "error got select("
                                                 << i << ") = " << got
                                                 << " but expected "
                                                 << expected);
        }
    };

    rank();
    select();

    static constexpr uint64_t num_flips = 5000;
    splitmix64 distr(essentials::get_random_seed());
    essentials::logger("flipping some bits...");
    for (uint64_t i = 0; i != num_flips; ++i) {
        uint64_t pos = distr.next() % bitmap.size();
        bitmap.flip(pos);
    }

    rank();
    select();

    std::cout << "\teverything's good" << std::endl;
}

}  // namespace mrs::testing
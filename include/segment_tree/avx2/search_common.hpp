#pragma once

namespace dyrs::avx2 {

inline int ctzll(unsigned long long w) {
    // the if is necessary since result is undefined otherwise,
    // although on most architectures it correctly returns 64
    if (w == 0) return 64;
    return __builtin_ctzll(w);
}

// find index of "first set" (fs) -- 128-bit version
template <uint64_t word_bits>  // 8, 16, 32, 64
inline static uint64_t index_fs(__m128i const& vec) {
    uint64_t const* w = reinterpret_cast<uint64_t const*>(&vec);
    uint64_t index = (ctzll(w[0]) + ctzll(w[1])) / word_bits;
    return index;
}

// find index of "first set" (fs) -- 256-bit version
template <uint64_t word_bits>  // 8, 16, 32, 64
inline static uint64_t index_fs(__m256i const& vec) {
    uint64_t const* w = reinterpret_cast<uint64_t const*>(&vec);
    uint64_t index =
        (ctzll(w[0]) + ctzll(w[1]) + ctzll(w[2]) + ctzll(w[3])) / word_bits;
    return index;
}

}  // namespace dyrs::avx2
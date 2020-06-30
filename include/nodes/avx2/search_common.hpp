#pragma once

namespace dyrs {

// find index of "first set" (fs) -- 128-bit version
template <uint64_t word_bits>  // 8, 16, 32, 64
inline static uint64_t index_fs(__m128i const& vec) {
    uint64_t w0 = _mm_extract_epi64(vec, 0);
    uint64_t w1 = _mm_extract_epi64(vec, 1);
    uint64_t index = (__builtin_ctzll(w0) + __builtin_ctzll(w1)) / word_bits;
    return index;
}

// find index of "first set" (fs) -- 256-bit version
template <uint64_t word_bits>  // 8, 16, 32, 64
inline static uint64_t index_fs(__m256i const& vec) {
    uint64_t w0 = _mm256_extract_epi64(vec, 0);
    uint64_t w1 = _mm256_extract_epi64(vec, 1);
    uint64_t w2 = _mm256_extract_epi64(vec, 2);
    uint64_t w3 = _mm256_extract_epi64(vec, 3);
    uint64_t index = (__builtin_ctzll(w0) + __builtin_ctzll(w1) +
                      __builtin_ctzll(w2) + __builtin_ctzll(w3)) /
                     word_bits;
    return index;
}

}  // namespace dyrs
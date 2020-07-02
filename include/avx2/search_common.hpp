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
    uint64_t w0 = _mm_extract_epi64(vec, 0);
    uint64_t w1 = _mm_extract_epi64(vec, 1);
    uint64_t index = (ctzll(w0) + ctzll(w1)) / word_bits;
    return index;
}

// find index of "first set" (fs) -- 256-bit version
template <uint64_t word_bits>  // 8, 16, 32, 64
inline static uint64_t index_fs(__m256i const& vec) {
    uint64_t w0 = _mm256_extract_epi64(vec, 0);
    uint64_t w1 = _mm256_extract_epi64(vec, 1);
    uint64_t w2 = _mm256_extract_epi64(vec, 2);
    uint64_t w3 = _mm256_extract_epi64(vec, 3);
    uint64_t index =
        (ctzll(w0) + ctzll(w1) + ctzll(w2) + ctzll(w3)) / word_bits;
    return index;
}

}  // namespace dyrs::avx2
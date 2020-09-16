#pragma once

#include "common.hpp"

namespace dyrs {

/* * * * * * * * * * * * * * * * * *
 *
 *   Rank256/512 approaches
 *
 * * * * * * * * * * * * * * * * */
enum class rank_modes : int {
    broadword_loop = int(popcount_modes::broadword) |  //
                     int(prefixsum_modes::loop) << 8,
    broadword_unrolled = int(popcount_modes::broadword) |  //
                         int(prefixsum_modes::unrolled) << 8,
#ifdef __SSE4_2__
    builtin_loop = int(popcount_modes::builtin) |  //
                   int(prefixsum_modes::loop) << 8,
    builtin_unrolled = int(popcount_modes::builtin) |  //
                       int(prefixsum_modes::unrolled) << 8,
#endif
#ifdef __AVX2__
    avx2_loop = int(popcount_modes::avx2) |  //
                int(prefixsum_modes::loop) << 8,
    avx2_unrolled = int(popcount_modes::avx2) |  //
                    int(prefixsum_modes::unrolled) << 8,
#endif
#ifdef __AVX512VL__
    builtin_parallel = int(popcount_modes::builtin) |  //
                       int(prefixsum_modes::parallel) << 8,

    avx2_parallel = int(popcount_modes::avx2) |  //
                    int(prefixsum_modes::parallel) << 8,

    avx512_loop = int(popcount_modes::avx512) |  //
                  int(prefixsum_modes::loop) << 8,
    avx512_unrolled = int(popcount_modes::avx512) |  //
                      int(prefixsum_modes::unrolled) << 8,
    avx512_parallel = int(popcount_modes::avx512) |  //
                      int(prefixsum_modes::parallel) << 8,
#endif
};

constexpr popcount_modes extract_popcount_mode(rank_modes m) {
    return popcount_modes(int(m) & 0xFF);
}
constexpr prefixsum_modes extract_prefixsum_mode(rank_modes m) {
    return prefixsum_modes(int(m) >> 8 & 0xFF);
}

static const std::map<rank_modes, std::string> rank_mode_map = {
    {rank_modes::broadword_loop, "broadword_loop"},          //
    {rank_modes::broadword_unrolled, "broadword_unrolled"},  //
#ifdef __SSE4_2__
    {rank_modes::builtin_loop, "builtin_loop"},          //
    {rank_modes::builtin_unrolled, "builtin_unrolled"},  //
#endif
#ifdef __AVX2__
    {rank_modes::avx2_loop, "avx2_loop"},          //
    {rank_modes::avx2_unrolled, "avx2_unrolled"},  //
#endif
#ifdef __AVX512VL__
    {rank_modes::builtin_parallel, "builtin_parallel"},  //
    {rank_modes::avx512_loop, "avx512_loop"},            //
    {rank_modes::avx2_parallel, "avx2_parallel"},        //
    {rank_modes::avx512_unrolled, "avx512_unrolled"},    //
    {rank_modes::avx512_parallel, "avx512_parallel"},    //
#endif
};

inline std::string print_rank_mode(rank_modes mode) {
    return rank_mode_map.find(mode)->second;
}

/* * * * * * * * * * * * * * * * * *
 *   Rank256 implementations
 * */
template <rank_modes Mode>
inline uint64_t rank_u256(const uint64_t* x, uint64_t i) {
    assert(i < 256);

    static constexpr auto pcm = extract_popcount_mode(Mode);
    static constexpr auto psm = extract_prefixsum_mode(Mode);

    const uint64_t block = i / 64;
    const uint64_t offset = (i + 1) & 63;
    const uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    const uint64_t rank_in_block = popcount_u64<pcm>(x[block] & mask);

    if constexpr (psm == prefixsum_modes::loop) {
        uint64_t sum = 0;
        if (block) {
            for (uint64_t b = 0; b <= block - 1; ++b) {
                sum += popcount_u64<pcm>(x[b]);
            }
        }
        return sum + rank_in_block;
    } else if constexpr (psm == prefixsum_modes::unrolled) {
        static uint64_t counts[4];
        counts[0] = 0;
        counts[1] = popcount_u64<pcm>(x[0]);
        counts[2] = counts[1] + popcount_u64<pcm>(x[1]);
        counts[3] = counts[2] + popcount_u64<pcm>(x[2]);
        return counts[block] + rank_in_block;
    } else {
        assert(false);
        return 0;
    }
}
#ifdef __AVX2__
template <>
inline uint64_t rank_u256<rank_modes::avx2_loop>(const uint64_t* x,
                                                 uint64_t i) {
    assert(i < 256);
    const uint64_t block = i / 64;
    const uint64_t offset = (i + 1) & 63;
    const uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    const uint64_t rank_in_block =
        popcount_u64<popcount_modes::builtin>(x[block] & mask);
    const __m256i counts =
        popcount_m256i(_mm256_loadu_si256((__m256i const*)x));
    const uint64_t* C = reinterpret_cast<uint64_t const*>(&counts);
    uint64_t sum = 0;
    if (block) {
        for (uint64_t b = 0; b <= block - 1; ++b) sum += C[b];
    }
    return sum + rank_in_block;
}
template <>
inline uint64_t rank_u256<rank_modes::avx2_unrolled>(const uint64_t* x,
                                                     uint64_t i) {
    assert(i < 256);

    const uint64_t block = i / 64;
    const uint64_t offset = (i + 1) & 63;
    const uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    const uint64_t rank_in_block =
        popcount_u64<popcount_modes::builtin>(x[block] & mask);

    const __m256i mcnts = popcount_m256i(_mm256_loadu_si256((__m256i const*)x));
    const uint64_t* C = reinterpret_cast<uint64_t const*>(&mcnts);

    static uint64_t counts[4];
    counts[0] = 0;
    counts[1] = C[0];
    counts[2] = counts[1] + C[1];
    counts[3] = counts[2] + C[2];
    return counts[block] + rank_in_block;
}
#endif

#ifdef __AVX512VL__
template <>
inline uint64_t rank_u256<rank_modes::builtin_parallel>(const uint64_t* x,
                                                        uint64_t i) {
    assert(i < 256);

    static uint64_t cnts[4];
    cnts[0] = popcount_u64<popcount_modes::builtin>(x[0]);
    cnts[1] = popcount_u64<popcount_modes::builtin>(x[1]);
    cnts[2] = popcount_u64<popcount_modes::builtin>(x[2]);
    cnts[3] = popcount_u64<popcount_modes::builtin>(x[3]);

    const uint64_t block = i / 64;
    const uint64_t offset = (i + 1) & 63;
    const uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    const uint64_t rank_in_block =
        popcount_u64<popcount_modes::builtin>(x[block] & mask);

    const __m256i msums =
        prefixsum_m256i(_mm256_loadu_si256((__m256i const*)cnts));

    static uint64_t sums[5] = {0ULL};  // the head element is a sentinel
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(sums + 1), msums);
    return sums[block] + rank_in_block;
}
template <>
inline uint64_t rank_u256<rank_modes::avx2_parallel>(const uint64_t* x,
                                                     uint64_t i) {
    assert(i < 256);

    const uint64_t block = i / 64;
    const uint64_t offset = (i + 1) & 63;
    const uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    const uint64_t rank_in_block =
        popcount_u64<popcount_modes::builtin>(x[block] & mask);

    const __m256i mcnts = popcount_m256i(_mm256_loadu_si256((__m256i const*)x));
    const __m256i msums = prefixsum_m256i(mcnts);

    static uint64_t sums[5] = {0ULL};  // the head element is a sentinel
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(sums + 1), msums);
    return sums[block] + rank_in_block;
}
#endif

/* * * * * * * * * * * * * * * * * *
 *   Rank512 implementations
 * */
template <rank_modes Mode>
inline uint64_t rank_u512(const uint64_t* x, uint64_t i) {
    assert(i < 512);

    static constexpr auto pcm = extract_popcount_mode(Mode);
    static constexpr auto psm = extract_prefixsum_mode(Mode);

    const uint64_t block = i / 64;
    const uint64_t offset = (i + 1) & 63;
    const uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    const uint64_t rank_in_block = popcount_u64<pcm>(x[block] & mask);

    if constexpr (psm == prefixsum_modes::loop) {
        uint64_t sum = 0;
        if (block) {
            for (uint64_t b = 0; b <= block - 1; ++b) {
                sum += popcount_u64<pcm>(x[b]);
            }
        }
        return sum + rank_in_block;
    } else if constexpr (psm == prefixsum_modes::unrolled) {
        static uint64_t counts[8];
        counts[0] = 0;
        counts[1] = popcount_u64<pcm>(x[0]);
        counts[2] = counts[1] + popcount_u64<pcm>(x[1]);
        counts[3] = counts[2] + popcount_u64<pcm>(x[2]);
        counts[4] = counts[3] + popcount_u64<pcm>(x[3]);
        counts[5] = counts[4] + popcount_u64<pcm>(x[4]);
        counts[6] = counts[5] + popcount_u64<pcm>(x[5]);
        counts[7] = counts[6] + popcount_u64<pcm>(x[6]);
        return counts[block] + rank_in_block;
    } else {
        assert(false);
        return 0;
    }
}
#ifdef __AVX512VL__
template <>
inline uint64_t rank_u512<rank_modes::avx512_loop>(const uint64_t* x,
                                                   uint64_t i) {
    assert(i < 256);
    const uint64_t block = i / 64;
    const uint64_t offset = (i + 1) & 63;
    const uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    const uint64_t rank_in_block =
        popcount_u64<popcount_modes::builtin>(x[block] & mask);
    const __m256i counts =
        popcount_m512i(_mm512_loadu_si512((__m256i const*)x));
    const uint64_t* C = reinterpret_cast<uint64_t const*>(&counts);
    uint64_t sum = 0;
    if (block) {
        for (uint64_t b = 0; b <= block - 1; ++b) sum += C[b];
    }
    return sum + rank_in_block;
}
template <>
inline uint64_t rank_u512<rank_modes::avx512_unrolled>(const uint64_t* x,
                                                       uint64_t i) {
    assert(i < 512);

    const uint64_t block = i / 64;
    const uint64_t offset = (i + 1) & 63;
    const uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    const uint64_t rank_in_block =
        popcount_u64<popcount_modes::builtin>(x[block] & mask);

    const __m512i mcnts = popcount_m512i(_mm512_loadu_si512((__m512i const*)x));
    const uint64_t* C = reinterpret_cast<uint64_t const*>(&mcnts);

    static uint64_t counts[8];
    counts[0] = 0;
    counts[1] = C[0];
    counts[2] = counts[1] + C[1];
    counts[3] = counts[2] + C[2];
    counts[4] = counts[3] + C[3];
    counts[5] = counts[4] + C[4];
    counts[6] = counts[5] + C[5];
    counts[7] = counts[6] + C[6];
    return counts[block] + rank_in_block;
}
template <>
inline uint64_t rank_u512<rank_modes::avx512_parallel>(const uint64_t* x,
                                                       uint64_t i) {
    assert(i < 512);

    const uint64_t block = i / 64;
    const uint64_t offset = (i + 1) & 63;
    const uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    const uint64_t rank_in_block =
        popcount_u64<popcount_modes::builtin>(x[block] & mask);

    const __m512i mcnts = popcount_m512i(_mm512_loadu_si512((__m512i const*)x));
    const __m512i msums = prefixsum_m512i(mcnts);

    static uint64_t sums[9] = {0ULL};  // the head element is a sentinel
    _mm512_storeu_si512(reinterpret_cast<__m512i*>(sums + 1), msums);
    return sums[block] + rank_in_block;
}
#endif

}  // namespace dyrs
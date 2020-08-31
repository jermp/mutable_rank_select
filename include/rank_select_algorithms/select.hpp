#pragma once

#include "common.hpp"

namespace dyrs {

/* * * * * * * * * * * * * * * * * *
 *
 *   Select64 approaches
 *
 * * * * * * * * * * * * * * * * */
enum class select64_modes : int {
    broadword_sdsl = 0x00,
    broadword_succinct = 0x01,
    sse4_sdsl = 0x02,
    sse4_succinct = 0x03,
    pdep = 0x04,
};
template <select64_modes>
inline uint64_t select_u64(uint64_t, uint64_t) {
    assert(false);
    return 0;  // should not come
}
// From https://github.com/simongog/sdsl-lite/blob/master/include/sdsl/bits.hpp
template <>
inline uint64_t select_u64<select64_modes::broadword_sdsl>(uint64_t x,
                                                           uint64_t i) {
    i += 1;

    uint64_t s = x, b;  // s = sum
    s = s - ((s >> 1) & 0x5555555555555555ULL);
    s = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
    s = (s + (s >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    s = 0x0101010101010101ULL * s;
    b = (s + ps_overflow[i]);  //&0x8080808080808080ULL;// add something to the
                               // partial sums to cause overflow
    i = (i - 1) << 8;
    if (b & 0x0000000080000000ULL)      // byte <=3
        if (b & 0x0000000000008000ULL)  // byte <= 1
            if (b & 0x0000000000000080ULL)
                return lt_sel[(x & 0xFFULL) + i];
            else
                return 8 + lt_sel[(((x >> 8) & 0xFFULL) + i -
                                   ((s & 0xFFULL) << 8)) &
                                  0x7FFULL];  // byte 1;
        else                                  // byte >1
            if (b & 0x0000000000800000ULL)    // byte <=2
            return 16 + lt_sel[(((x >> 16) & 0xFFULL) + i - (s & 0xFF00ULL)) &
                               0x7FFULL];  // byte 2;
        else
            return 24 +
                   lt_sel[(((x >> 24) & 0xFFULL) + i - ((s >> 8) & 0xFF00ULL)) &
                          0x7FFULL];    // byte 3;
    else                                //  byte > 3
        if (b & 0x0000800000000000ULL)  // byte <=5
        if (b & 0x0000008000000000ULL)  // byte <=4
            return 32 + lt_sel[(((x >> 32) & 0xFFULL) + i -
                                ((s >> 16) & 0xFF00ULL)) &
                               0x7FFULL];  // byte 4;
        else
            return 40 + lt_sel[(((x >> 40) & 0xFFULL) + i -
                                ((s >> 24) & 0xFF00ULL)) &
                               0x7FFULL];  // byte 5;
    else                                   // byte >5
        if (b & 0x0080000000000000ULL)     // byte<=6
        return 48 +
               lt_sel[(((x >> 48) & 0xFFULL) + i - ((s >> 32) & 0xFF00ULL)) &
                      0x7FFULL];  // byte 6;
    else
        return 56 +
               lt_sel[(((x >> 56) & 0xFFULL) + i - ((s >> 40) & 0xFF00ULL)) &
                      0x7FFULL];  // byte 7;
    return 0;
}
// From https://github.com/ot/succinct/blob/master/broadword.hpp
template <>
inline uint64_t select_u64<select64_modes::broadword_succinct>(uint64_t x,
                                                               uint64_t k) {
    assert(k < popcount_u64<popcount_modes::broadword>(x));

    uint64_t byte_sums = byte_counts(x) * ones_step_8;
    const uint64_t k_step_8 = k * ones_step_8;
    const uint64_t geq_k_step_8 =
        (((k_step_8 | msbs_step_8) - byte_sums) & msbs_step_8);
    const uint64_t place =
        ((geq_k_step_8 >> 7) * ones_step_8 >> 53) & ~uint64_t(0x7);
    const uint64_t byte_rank =
        k - (((byte_sums << 8) >> place) & uint64_t(0xFF));
    return place + select_in_byte[((x >> place) & 0xFF) | (byte_rank << 8)];
}
#ifdef __SSE4_2__
// From https://github.com/simongog/sdsl-lite/blob/master/include/sdsl/bits.hpp
template <>
inline uint64_t select_u64<select64_modes::sse4_sdsl>(uint64_t x, uint64_t i) {
    i += 1;

    uint64_t s = x, b;
    s = s - ((s >> 1) & 0x5555555555555555ULL);
    s = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
    s = (s + (s >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    s = 0x0101010101010101ULL * s;
    // now s contains 8 bytes s[7],...,s[0]; s[j] contains the cumulative sum
    // of (j+1)*8 least significant bits of s
    b = (s + ps_overflow[i]) & 0x8080808080808080ULL;
    // ps_overflow contains a bit mask x consisting of 8 bytes
    // x[7],...,x[0] and x[j] is set to 128-j
    // => a byte b[j] in b is >= 128 if cum sum >= j

    // __builtin_ctzll returns the number of trailing zeros, if b!=0
    int byte_nr = _tzcnt_u64(b) >> 3;  // byte nr in [0..7]
    s <<= 8;
    i -= (s >> (byte_nr << 3)) & 0xFFULL;
    return (byte_nr << 3) +
           lt_sel[((i - 1) << 8) + ((x >> (byte_nr << 3)) & 0xFFULL)];
}
// From https://github.com/ot/succinct/blob/master/broadword.hpp
template <>
inline uint64_t select_u64<select64_modes::sse4_succinct>(uint64_t x,
                                                          uint64_t k) {
    assert(k < popcount_u64<popcount_modes::broadword>(x));

    uint64_t byte_sums = byte_counts(x) * ones_step_8;
    const uint64_t k_step_8 = k * ones_step_8;
    const uint64_t geq_k_step_8 =
        (((k_step_8 | msbs_step_8) - byte_sums) & msbs_step_8);
    const uint64_t place =
        popcount_u64<popcount_modes::builtin>(geq_k_step_8) * 8;
    const uint64_t byte_rank =
        k - (((byte_sums << 8) >> place) & uint64_t(0xFF));
    return place + select_in_byte[((x >> place) & 0xFF) | (byte_rank << 8)];
}
#endif
#ifdef __BMI2__
template <>
inline uint64_t select_u64<select64_modes::pdep>(uint64_t x, uint64_t i) {
    return _tzcnt_u64(_pdep_u64(1ull << i, x));
}
#endif

/* * * * * * * * * * * * * * * * * *
 *
 *   Searching approaches
 *
 * * * * * * * * * * * * * * * * */
enum class search_modes : int {
    loop = 0x00,
    avx512 = 0x01,
};

/* * * * * * * * * * * * * * * * * *
 *
 *   Select256/512 approaches
 *
 * * * * * * * * * * * * * * * * */
enum class select_modes : int {
    broadword_loop_sdsl = int(popcount_modes::broadword) |  //
                          int(search_modes::loop) << 8 |    //
                          int(select64_modes::broadword_sdsl) << 16,
    broadword_loop_succinct = int(popcount_modes::broadword) |  //
                              int(search_modes::loop) << 8 |    //
                              int(select64_modes::broadword_succinct) << 16,
#ifdef __SSE4_2__
    builtin_loop_sdsl = int(popcount_modes::builtin) |  //
                        int(search_modes::loop) << 8 |  //
                        int(select64_modes::sse4_sdsl) << 16,
    builtin_loop_succinct = int(popcount_modes::builtin) |  //
                            int(search_modes::loop) << 8 |  //
                            int(select64_modes::sse4_succinct) << 16,
#endif
#ifdef __BMI2__
    builtin_loop_pdep = int(popcount_modes::builtin) |  //
                        int(search_modes::loop) << 8 |  //
                        int(select64_modes::pdep) << 16,
#endif
#ifdef __AVX512VL__
    builtin_avx512_pdep = int(popcount_modes::builtin) |    //
                          int(search_modes::avx512) << 8 |  //
                          int(select64_modes::pdep) << 16,
    avx2_avx512_pdep = int(popcount_modes::avx2) |       //
                       int(search_modes::avx512) << 8 |  //
                       int(select64_modes::pdep) << 16,
    avx512_avx512_pdep = int(popcount_modes::avx512) |     //
                         int(search_modes::avx512) << 8 |  //
                         int(select64_modes::pdep) << 16,
#endif
};

constexpr popcount_modes extract_popcount_mode(select_modes m) {
    return popcount_modes(int(m) & 0xFF);
}
constexpr search_modes extract_search_mode(select_modes m) {
    return search_modes(int(m) >> 8 & 0xFF);
}
constexpr select64_modes extract_select64_mode(select_modes m) {
    return select64_modes(int(m) >> 16 & 0xFF);
}

static const std::map<select_modes, std::string> select_mode_map = {
    {select_modes::broadword_loop_sdsl, "broadword_loop_sdsl"},          //
    {select_modes::broadword_loop_succinct, "broadword_loop_succinct"},  //
#ifdef __SSE4_2__
    {select_modes::builtin_loop_sdsl, "builtin_loop_sdsl"},          //
    {select_modes::builtin_loop_succinct, "builtin_loop_succinct"},  //
#endif
#ifdef __BMI2__
    {select_modes::builtin_loop_pdep, "builtin_loop_pdep"},  //
#endif
#ifdef __AVX512VL__
    {select_modes::builtin_avx512_pdep, "builtin_avx512_pdep"},  //
    {select_modes::avx2_avx512_pdep, "avx2_avx512_pdep"},        //
    {select_modes::avx512_avx512_pdep, "avx512_avx512_pdep"},    //
#endif
};

inline std::string print_select_mode(select_modes mode) {
    return select_mode_map.find(mode)->second;
}

/* * * * * * * * * * * * * * * * * *
 *   Select256 implementations
 * * * * * * * * * * * * * * * * */
template <select_modes Mode>
inline uint64_t select_u256(const uint64_t* x, uint64_t k) {
    static constexpr auto pcm = extract_popcount_mode(Mode);
    static constexpr auto scm = extract_search_mode(Mode);
    static constexpr auto swm = extract_select64_mode(Mode);
#ifdef __SSE__
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);
#endif
    if constexpr (scm == search_modes::loop) {
        uint64_t i = 0, cnt = 0;
        while (i < 4) {
            cnt = popcount_u64<pcm>(x[i]);
            if (k < cnt) { break; }
            k -= cnt;
            i++;
        }
        assert(k < cnt);
        return i * 64 + select_u64<swm>(x[i], k);
    } else {
        assert(false);
        return 0;
    }
}
#ifdef __AVX512VL__
template <>
inline uint64_t select_u256<select_modes::builtin_avx512_pdep>(
    const uint64_t* x, uint64_t k) {
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);

    static uint64_t cnts[4];
    cnts[0] = popcount_u64<popcount_modes::builtin>(x[0]);
    cnts[1] = popcount_u64<popcount_modes::builtin>(x[1]);
    cnts[2] = popcount_u64<popcount_modes::builtin>(x[2]);
    cnts[3] = popcount_u64<popcount_modes::builtin>(x[3]);

    const __m256i mcnts =
        _mm256_loadu_si256(reinterpret_cast<const __m256i*>(cnts));
    const __m256i msums = prefixsum_m256i(mcnts);
    const __m256i mk = _mm256_set_epi64x(k, k, k, k);
    const __mmask8 mask = _mm256_cmple_epi64_mask(msums, mk);  // 1 if mc <= mk
    const uint8_t i = lt_cnt[mask];

    static uint64_t sums[5] = {0ULL};  // the head element is a sentinel
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(sums + 1), msums);

    return i * 64 + select_u64<select64_modes::pdep>(x[i], k - sums[i]);
}
template <>
inline uint64_t select_u256<select_modes::avx2_avx512_pdep>(const uint64_t* x,
                                                            uint64_t k) {
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);

    const __m256i mx = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(x));
    const __m256i msums = prefixsum_m256i(popcount_m256i(mx));
    const __m256i mk = _mm256_set_epi64x(k, k, k, k);
    const __mmask8 mask = _mm256_cmple_epi64_mask(msums, mk);  // 1 if mc <= mk
    const uint8_t i = lt_cnt[mask];

    static uint64_t sums[5] = {0ULL};  // the head element is a sentinel
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(sums + 1), msums);

    return i * 64 + select_u64<select64_modes::pdep>(x[i], k - sums[i]);
}
#endif

/* * * * * * * * * * * * * * * * * *
 *   Select512 implementations
 * * * * * * * * * * * * * * * * */
template <select_modes Mode>
inline uint64_t select_u512(const uint64_t* x, uint64_t k) {
    static constexpr auto pcm = extract_popcount_mode(Mode);
    static constexpr auto scm = extract_search_mode(Mode);
    static constexpr auto swm = extract_select64_mode(Mode);
#ifdef __SSE__
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);
#endif
    if constexpr (scm == search_modes::loop) {
        uint64_t i = 0, cnt = 0;
        while (i < 8) {
            cnt = popcount_u64<pcm>(x[i]);
            if (k < cnt) { break; }
            k -= cnt;
            i++;
        }
        assert(k < cnt);
        return i * 64 + select_u64<swm>(x[i], k);
    } else {
        assert(false);
        return 0;
    }
}
#ifdef __AVX512VL__
template <>
inline uint64_t select_u512<select_modes::builtin_avx512_pdep>(
    const uint64_t* x, uint64_t k) {
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);

    static uint64_t cnts[8];
    cnts[0] = popcount_u64<popcount_modes::builtin>(x[0]);
    cnts[1] = popcount_u64<popcount_modes::builtin>(x[1]);
    cnts[2] = popcount_u64<popcount_modes::builtin>(x[2]);
    cnts[3] = popcount_u64<popcount_modes::builtin>(x[3]);
    cnts[4] = popcount_u64<popcount_modes::builtin>(x[4]);
    cnts[5] = popcount_u64<popcount_modes::builtin>(x[5]);
    cnts[6] = popcount_u64<popcount_modes::builtin>(x[6]);
    cnts[7] = popcount_u64<popcount_modes::builtin>(x[7]);

    const __m512i mcnts =
        _mm512_loadu_si512(reinterpret_cast<const __m512i*>(cnts));
    const __m512i msums = prefixsum_m512i(mcnts);
    const __m512i mk = _mm512_set_epi64(k, k, k, k, k, k, k, k);
    const __mmask8 mask = _mm512_cmple_epi64_mask(msums, mk);  // 1 if mc <= mk
    const uint8_t i = lt_cnt[mask];

    static uint64_t sums[9] = {0ULL};  // the head element is a sentinel
    _mm512_storeu_si512(reinterpret_cast<__m512i*>(sums + 1), msums);

    return i * 64 + select_u64<select64_modes::pdep>(x[i], k - sums[i]);
}
template <>
inline uint64_t select_u512<select_modes::avx512_avx512_pdep>(const uint64_t* x,
                                                              uint64_t k) {
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);

    const __m512i mx = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(x));
    const __m512i msums = prefixsum_m512i(popcount_m512i(mx));
    const __m512i mk = _mm512_set_epi64(k, k, k, k, k, k, k, k);
    const __mmask8 mask = _mm512_cmple_epi64_mask(msums, mk);  // 1 if mc <= mk
    const uint8_t i = lt_cnt[mask];

    static uint64_t sums[9] = {0ULL};  // the head element is a sentinel
    _mm512_storeu_si512(reinterpret_cast<__m512i*>(sums + 1), msums);

    return i * 64 + select_u64<select64_modes::pdep>(x[i], k - sums[i]);
}
#endif

}  // namespace dyrs
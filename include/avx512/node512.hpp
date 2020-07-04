#pragma once

#include <cassert>

#include "immintrin.h"
#include "tables.hpp"
#include "../util.hpp"

namespace dyrs::avx512 {

struct node512 {
    typedef uint16_t key_type;  // each key should be an integer in [0,2^8]
    typedef uint32_t summary_type;
    static constexpr uint64_t fanout = 512;
    static constexpr uint64_t segment_size = 32;
    static constexpr uint64_t num_segments = 16;
    static constexpr uint64_t bytes =
        fanout * sizeof(key_type) + num_segments * sizeof(summary_type);

    node512() {}

    static void build(key_type const* input, uint8_t* out) {
        build_node_prefix_sums<summary_type, key_type>(input, out, segment_size,
                                                       num_segments, bytes);
    }

    static std::string name() {
        return "avx512::node512";
    }

    node512(uint8_t* ptr) {
        at(ptr);
    }

    inline void at(uint8_t* ptr) {
        summary = reinterpret_cast<summary_type*>(ptr);
        keys = reinterpret_cast<key_type*>(ptr +
                                           num_segments * sizeof(summary_type));
    }

    void update(uint64_t i, int8_t delta) {
        if (i == fanout) return;
        assert(i < fanout);
        assert(delta == +1 or delta == -1);
        uint64_t j = i / segment_size;
        uint64_t k = i % segment_size;
#ifdef AVX512
        bool sign = delta >> 7;
        __m512i s1 = _mm512_load_si512((__m512i const*)tables::update_16_32 +
                                       j + 1 + sign * (num_segments + 1));
        __m512i r1 =
            _mm512_add_epi32(_mm512_loadu_si512((__m512i const*)summary), s1);
        _mm512_storeu_si512((__m512i*)summary, r1);
        __m512i s2 = _mm512_load_si512((__m512i const*)tables::update_32_16 +
                                       k + sign * segment_size);
        __m512i r2 = _mm512_add_epi16(
            _mm512_loadu_si512((__m512i const*)(keys + j * segment_size)), s2);
        _mm512_storeu_si512((__m512i*)(keys + j * segment_size), r2);
#else
        for (uint64_t z = j + 1; z != num_segments; ++z) summary[z] += delta;
        for (uint64_t z = k, base = j * segment_size; z != segment_size; ++z) {
            keys[base + z] += delta;
        }
#endif
    }

    uint64_t sum(uint64_t i) const {
        assert(i < fanout);
        return summary[i / segment_size] + keys[i];
    }

    /* return the smallest i in [0,fanout-1] such that sum(i) > x;
       in this case x is always < sum(fanout-1) by assumption */
    uint64_t search(uint64_t x) const {
        assert(x < sum(fanout - 1));
        uint64_t i = 0;
#ifdef AVX512
        __mmask16 cmp1 = _mm512_cmpgt_epi32_mask(
            _mm512_loadu_si512((__m512i const*)summary), _mm512_set1_epi32(x));
        i = cmp1 != 0 ? __builtin_ctz(cmp1) - 1 : num_segments - 1;
        assert(i < num_segments);
        x -= summary[i];
        i *= segment_size;
        __mmask32 cmp2 = _mm512_cmpgt_epi16_mask(
            _mm512_loadu_si512((__m512i const*)(keys + i)),
            _mm512_set1_epi16(x));
        assert(cmp2 > 0);
        i += __builtin_ctz(cmp2);
#else
        for (uint64_t z = 1; z != num_segments; ++z, ++i) {
            if (summary[z] > x) break;
        }
        assert(i < num_segments);
        x -= summary[i];
        i *= segment_size;
        for (uint64_t z = 0; z != segment_size; ++z, ++i) {
            if (keys[i] > x) break;
        }
#endif
        assert(i < fanout);
        return i;
    }

private:
    summary_type* summary;
    key_type* keys;
};

}  // namespace dyrs::avx512
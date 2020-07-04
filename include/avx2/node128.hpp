#pragma once

#include <cassert>

#include "immintrin.h"
#include "tables.hpp"
#include "search_common.hpp"
#include "../util.hpp"

namespace dyrs::avx2 {

struct node128 {
    typedef uint16_t key_type;  // each key should be an integer in [0,2^8]
    typedef uint16_t summary_type;
    static constexpr uint64_t fanout = 128;
    static constexpr uint64_t segment_size = 16;
    static constexpr uint64_t num_segments = 8;
    static constexpr uint64_t bytes =
        fanout * sizeof(key_type) + num_segments * sizeof(summary_type);

    node128() {}

    static void build(key_type const* input, uint8_t* out) {
        build_node_prefix_sums<summary_type, key_type>(input, out, segment_size,
                                                       num_segments, bytes);
    }

    static std::string name() {
        return "avx2::node128";
    }

    node128(uint8_t* ptr) {
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
#ifdef DISABLE_AVX
        for (uint64_t z = j + 1; z != num_segments; ++z) summary[z] += delta;
        for (uint64_t z = k, base = j * segment_size; z != segment_size; ++z) {
            keys[base + z] += delta;
        }
#else
        bool sign = delta >> 7;
        __m128i s1 = _mm_load_si128((__m128i const*)tables::update_8_16 + j +
                                    sign * num_segments);
        __m128i r1 =
            _mm_add_epi16(_mm_loadu_si128((__m128i const*)summary), s1);
        _mm_storeu_si128((__m128i*)summary, r1);
        __m256i s2 = _mm256_load_si256((__m256i const*)tables::update_16_16 +
                                       k + sign * segment_size);
        __m256i r2 = _mm256_add_epi16(
            _mm256_loadu_si256((__m256i const*)(keys + j * segment_size)), s2);
        _mm256_storeu_si256((__m256i*)(keys + j * segment_size), r2);
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
#ifdef DISABLE_AVX
        for (uint64_t z = 1; z != num_segments; ++z, ++i) {
            if (summary[z] > x) break;
        }
        assert(i < num_segments);
        x -= summary[i];
        i *= segment_size;
        for (uint64_t z = 0; z != segment_size; ++z, ++i) {
            if (keys[i] > x) break;
        }
#else
        __m128i cmp1 = _mm_cmpgt_epi16(_mm_loadu_si128((__m128i const*)summary),
                                       _mm_set1_epi16(x));
        i = index_fs<16>(cmp1) - 1;
        assert(i < num_segments);
        x -= summary[i];
        i *= segment_size;
        __m256i cmp2 =
            _mm256_cmpgt_epi16(_mm256_loadu_si256((__m256i const*)(keys + i)),
                               _mm256_set1_epi16(x));
        i += index_fs<16>(cmp2);
#endif
        assert(i < fanout);
        return i;
    }

private:
    summary_type* summary;
    key_type* keys;
};

}  // namespace dyrs::avx2
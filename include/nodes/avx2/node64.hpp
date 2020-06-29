#pragma once

#include <cassert>

#include "immintrin.h"
#include "../common.hpp"
#include "../tables.hpp"

namespace dyrs {

struct node64 {
    typedef uint32_t key_type;  // each key should be an integer in [0,2^21]
    typedef uint32_t summary_type;
    static constexpr uint64_t fanout = 64;
    static constexpr uint64_t segment_size = 8;
    static constexpr uint64_t num_segments = 8;
    static constexpr uint64_t bytes =
        fanout * sizeof(key_type) + num_segments * sizeof(summary_type);

    node64() {}

    static void build(key_type const* input, uint8_t* out) {
        build_node_prefix_sums<summary_type, key_type>(input, out, segment_size,
                                                       num_segments, bytes);
    }

    static std::string name() {
        return "node64";
    }

    node64(uint8_t* ptr) {
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

        __m256i s1 =
            _mm256_load_si256((__m256i const*)tables::avx2::update_8_32 + j +
                              1 + sign * (num_segments + 1));
        __m256i r1 =
            _mm256_add_epi32(_mm256_loadu_si256((__m256i const*)summary), s1);
        _mm256_storeu_si256((__m256i*)summary, r1);

        __m256i s2 =
            _mm256_load_si256((__m256i const*)tables::avx2::update_8_32 + k +
                              sign * (segment_size + 1));
        __m256i r2 = _mm256_add_epi32(
            _mm256_loadu_si256((__m256i const*)(keys + j * segment_size)), s2);
        _mm256_storeu_si256((__m256i*)(keys + j * segment_size), r2);
#endif
    }

    uint64_t sum(uint64_t i) const {
        assert(i < fanout);
        return summary[i / segment_size] + keys[i];
    }

private:
    summary_type* summary;
    key_type* keys;
};

}  // namespace dyrs
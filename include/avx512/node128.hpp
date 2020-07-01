#pragma once

#include <cassert>

#include "immintrin.h"
#include "tables.hpp"
#include "../common.hpp"

namespace dyrs::avx512 {

struct node128 {
    typedef uint32_t key_type;  // each key should be an integer in [0,2^25]
    typedef uint32_t summary_type;
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
        return "avx512::node128";
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
#ifdef AVX512
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
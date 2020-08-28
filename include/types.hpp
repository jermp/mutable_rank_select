#pragma once

#include "segment_tree/avx2/node128.hpp"
#include "segment_tree/avx2/node64.hpp"
#include "segment_tree/avx2/node32.hpp"
#include "segment_tree/avx2/segment_tree.hpp"

#include "segment_tree/avx512/node512.hpp"
#include "segment_tree/avx512/node256.hpp"
#include "segment_tree/avx512/node128.hpp"
#include "segment_tree/avx512/segment_tree.hpp"

#include "mutable_bitmap.hpp"
#include "mutable_bitmap_64.hpp"

namespace dyrs {

template <rank_modes RankMode, select_modes SelectMode>
struct block256_type_traits {
    static constexpr uint64_t block_size = 256;
    static constexpr rank_modes rank_mode = RankMode;
    static constexpr select_modes select_mode = SelectMode;

    inline static uint64_t rank(const uint64_t* x, uint64_t i) {
        return rank_u256<RankMode>(x, i);
    }
    inline static uint64_t select(const uint64_t* x, uint64_t i) {
        return select_u256<SelectMode>(x, i);
    }
};

template <rank_modes RankMode, select_modes SelectMode>
struct block512_type_traits {
    static constexpr uint64_t block_size = 512;
    static constexpr rank_modes rank_mode = RankMode;
    static constexpr select_modes select_mode = SelectMode;

    inline static uint64_t rank(const uint64_t* x, uint64_t i) {
        return rank_u512<RankMode>(x, i);
    }
    inline static uint64_t select(const uint64_t* x, uint64_t i) {
        return select_u512<SelectMode>(x, i);
    }
};

typedef block256_type_traits<rank_modes::builtin_unrolled,
                             select_modes::builtin_loop_pdep>
    block256_type_1;

typedef block512_type_traits<rank_modes::builtin_unrolled,
                             select_modes::builtin_loop_pdep>
    block512_type_1;

#ifdef __AVX512VL__
typedef block256_type_traits<rank_modes::builtin_unrolled,
                             select_modes::avx2_avx512_pdep>
    block256_type_2;
#endif

}  // namespace dyrs
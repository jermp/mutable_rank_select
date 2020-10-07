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

namespace mrs {

template <rank_modes RankMode, select_modes SelectMode>
struct block64_type {
    static constexpr uint64_t block_size = 64;
    static constexpr rank_modes rank_mode = RankMode;
    static constexpr select_modes select_mode = SelectMode;

    inline static uint64_t rank(const uint64_t* x, uint64_t i) {
        i += 1;
        uint64_t mask = i == 64 ? -1 : (i != 0) * (1ULL << i) - 1;
        return popcount_u64<popcount_modes::builtin>(*x & mask);
    }
    inline static uint64_t select(const uint64_t* x, uint64_t i) {
        return select_u64<select64_modes::pdep>(*x, i);
    }
};

template <rank_modes RankMode, select_modes SelectMode>
struct block256_type {
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
struct block512_type {
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

typedef block64_type<rank_modes::builtin, select_modes::pdep>
    block64_type_default;

typedef block256_type<rank_modes::builtin_unrolled,
                      select_modes::builtin_loop_pdep>
    block256_type_a;

typedef block256_type<rank_modes::builtin_loop, select_modes::builtin_loop_pdep>
    block256_type_b;

typedef block512_type<rank_modes::builtin_unrolled,
                      select_modes::builtin_loop_pdep>
    block512_type_a;

typedef block512_type<rank_modes::builtin_loop, select_modes::builtin_loop_pdep>
    block512_type_b;

#ifdef __AVX512VL__
typedef block256_type<rank_modes::builtin_unrolled,
                      select_modes::avx2_avx512_pdep>
    block256_type_c;
#endif

}  // namespace mrs
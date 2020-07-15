#pragma once

#include "avx2/node128.hpp"
#include "avx2/node64.hpp"
#include "avx2/node32.hpp"
#include "avx2/segment_tree.hpp"

#include "avx512/node512.hpp"
#include "avx512/node256.hpp"
#include "avx512/node128.hpp"
#include "avx512/segment_tree.hpp"

#include "mutable_bitvector.hpp"
#include "mutable_bitvector_64.hpp"

namespace dyrs {

template <rank_modes RankMode, select_modes SelectMode>
struct rank_select_modes {
    static constexpr rank_modes rank_mode = RankMode;
    static constexpr select_modes select_mode = SelectMode;
};

typedef rank_select_modes<rank_modes::SSE4_2_POPCNT,
                          select_modes::BMI2_PDEP_TZCNT>
    rank_select_modes_1;

#ifdef __AVX512VL__
typedef rank_select_modes<rank_modes::SSE4_2_POPCNT,
                          select_modes::AVX2_POPCNT_AVX512_PREFIX_SUM>
    rank_select_modes_2;
#endif

}  // namespace dyrs
#pragma once

#include <stdint.h>
#include <cassert>
#include <string>
#include <map>

#include <immintrin.h>
#include <nmmintrin.h>

namespace dyrs {

// From https://github.com/simongog/sdsl-lite/blob/master/include/sdsl/bits.hpp
static const uint8_t lt_cnt[] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4,
    2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5,
    3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6,
    4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};

// From https://github.com/simongog/sdsl-lite/blob/master/include/sdsl/bits.hpp
static const uint64_t ps_overflow[] = {
    0x8080808080808080ULL, 0x7f7f7f7f7f7f7f7fULL, 0x7e7e7e7e7e7e7e7eULL,
    0x7d7d7d7d7d7d7d7dULL, 0x7c7c7c7c7c7c7c7cULL, 0x7b7b7b7b7b7b7b7bULL,
    0x7a7a7a7a7a7a7a7aULL, 0x7979797979797979ULL, 0x7878787878787878ULL,
    0x7777777777777777ULL, 0x7676767676767676ULL, 0x7575757575757575ULL,
    0x7474747474747474ULL, 0x7373737373737373ULL, 0x7272727272727272ULL,
    0x7171717171717171ULL, 0x7070707070707070ULL, 0x6f6f6f6f6f6f6f6fULL,
    0x6e6e6e6e6e6e6e6eULL, 0x6d6d6d6d6d6d6d6dULL, 0x6c6c6c6c6c6c6c6cULL,
    0x6b6b6b6b6b6b6b6bULL, 0x6a6a6a6a6a6a6a6aULL, 0x6969696969696969ULL,
    0x6868686868686868ULL, 0x6767676767676767ULL, 0x6666666666666666ULL,
    0x6565656565656565ULL, 0x6464646464646464ULL, 0x6363636363636363ULL,
    0x6262626262626262ULL, 0x6161616161616161ULL, 0x6060606060606060ULL,
    0x5f5f5f5f5f5f5f5fULL, 0x5e5e5e5e5e5e5e5eULL, 0x5d5d5d5d5d5d5d5dULL,
    0x5c5c5c5c5c5c5c5cULL, 0x5b5b5b5b5b5b5b5bULL, 0x5a5a5a5a5a5a5a5aULL,
    0x5959595959595959ULL, 0x5858585858585858ULL, 0x5757575757575757ULL,
    0x5656565656565656ULL, 0x5555555555555555ULL, 0x5454545454545454ULL,
    0x5353535353535353ULL, 0x5252525252525252ULL, 0x5151515151515151ULL,
    0x5050505050505050ULL, 0x4f4f4f4f4f4f4f4fULL, 0x4e4e4e4e4e4e4e4eULL,
    0x4d4d4d4d4d4d4d4dULL, 0x4c4c4c4c4c4c4c4cULL, 0x4b4b4b4b4b4b4b4bULL,
    0x4a4a4a4a4a4a4a4aULL, 0x4949494949494949ULL, 0x4848484848484848ULL,
    0x4747474747474747ULL, 0x4646464646464646ULL, 0x4545454545454545ULL,
    0x4444444444444444ULL, 0x4343434343434343ULL, 0x4242424242424242ULL,
    0x4141414141414141ULL, 0x4040404040404040ULL};

// From https://github.com/simongog/sdsl-lite/blob/master/include/sdsl/bits.hpp
static const uint8_t lt_sel[] = {
    0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0,
    3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0,
    3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0,
    3, 0, 1, 0, 2, 0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0,
    3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0,
    3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
    4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,

    0, 0, 0, 1, 0, 2, 2, 1, 0, 3, 3, 1, 3, 2, 2, 1, 0, 4, 4, 1, 4, 2, 2, 1,
    4, 3, 3, 1, 3, 2, 2, 1, 0, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1,
    5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 0, 6, 6, 1, 6, 2, 2, 1,
    6, 3, 3, 1, 3, 2, 2, 1, 6, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
    6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1,
    4, 3, 3, 1, 3, 2, 2, 1, 0, 7, 7, 1, 7, 2, 2, 1, 7, 3, 3, 1, 3, 2, 2, 1,
    7, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 7, 5, 5, 1, 5, 2, 2, 1,
    5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,
    7, 6, 6, 1, 6, 2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1, 6, 4, 4, 1, 4, 2, 2, 1,
    4, 3, 3, 1, 3, 2, 2, 1, 6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1,
    5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1,

    0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 3, 0, 3, 3, 2, 0, 0, 0, 4, 0, 4, 4, 2,
    0, 4, 4, 3, 4, 3, 3, 2, 0, 0, 0, 5, 0, 5, 5, 2, 0, 5, 5, 3, 5, 3, 3, 2,
    0, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2, 0, 0, 0, 6, 0, 6, 6, 2,
    0, 6, 6, 3, 6, 3, 3, 2, 0, 6, 6, 4, 6, 4, 4, 2, 6, 4, 4, 3, 4, 3, 3, 2,
    0, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2, 6, 5, 5, 4, 5, 4, 4, 2,
    5, 4, 4, 3, 4, 3, 3, 2, 0, 0, 0, 7, 0, 7, 7, 2, 0, 7, 7, 3, 7, 3, 3, 2,
    0, 7, 7, 4, 7, 4, 4, 2, 7, 4, 4, 3, 4, 3, 3, 2, 0, 7, 7, 5, 7, 5, 5, 2,
    7, 5, 5, 3, 5, 3, 3, 2, 7, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,
    0, 7, 7, 6, 7, 6, 6, 2, 7, 6, 6, 3, 6, 3, 3, 2, 7, 6, 6, 4, 6, 4, 4, 2,
    6, 4, 4, 3, 4, 3, 3, 2, 7, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2,
    6, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 4,
    0, 0, 0, 4, 0, 4, 4, 3, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5, 0, 5, 5, 3,
    0, 0, 0, 5, 0, 5, 5, 4, 0, 5, 5, 4, 5, 4, 4, 3, 0, 0, 0, 0, 0, 0, 0, 6,
    0, 0, 0, 6, 0, 6, 6, 3, 0, 0, 0, 6, 0, 6, 6, 4, 0, 6, 6, 4, 6, 4, 4, 3,
    0, 0, 0, 6, 0, 6, 6, 5, 0, 6, 6, 5, 6, 5, 5, 3, 0, 6, 6, 5, 6, 5, 5, 4,
    6, 5, 5, 4, 5, 4, 4, 3, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 3,
    0, 0, 0, 7, 0, 7, 7, 4, 0, 7, 7, 4, 7, 4, 4, 3, 0, 0, 0, 7, 0, 7, 7, 5,
    0, 7, 7, 5, 7, 5, 5, 3, 0, 7, 7, 5, 7, 5, 5, 4, 7, 5, 5, 4, 5, 4, 4, 3,
    0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 3, 0, 7, 7, 6, 7, 6, 6, 4,
    7, 6, 6, 4, 6, 4, 4, 3, 0, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 3,
    7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5,
    0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 5, 0, 5, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 4,
    0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 6, 0, 6, 6, 5, 0, 0, 0, 6, 0, 6, 6, 5,
    0, 6, 6, 5, 6, 5, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
    0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 4, 0, 0, 0, 0, 0, 0, 0, 7,
    0, 0, 0, 7, 0, 7, 7, 5, 0, 0, 0, 7, 0, 7, 7, 5, 0, 7, 7, 5, 7, 5, 5, 4,
    0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6, 0, 0, 0, 7, 0, 7, 7, 6,
    0, 7, 7, 6, 7, 6, 6, 4, 0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 5,
    0, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6, 5, 6, 5, 5, 4,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 6,
    0, 0, 0, 6, 0, 6, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 5,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 7,
    0, 0, 0, 7, 0, 7, 7, 6, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6,
    0, 0, 0, 7, 0, 7, 7, 6, 0, 7, 7, 6, 7, 6, 6, 5,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7,
    0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 7, 0, 7, 7, 6,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7};

// From https://github.com/ot/succinct/blob/master/broadword.hpp
static const uint8_t select_in_byte[2048] = {
    8, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3,
    0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0,
    1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1,
    0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0,
    2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2,
    0, 1, 0, 7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0,
    1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1,
    0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 6, 0, 1, 0, 2, 0, 1, 0,
    3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 5,
    0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0, 4, 0, 1, 0, 2, 0, 1, 0, 3, 0,
    1, 0, 2, 0, 1, 0, 8, 8, 8, 1, 8, 2, 2, 1, 8, 3, 3, 1, 3, 2, 2, 1, 8, 4, 4,
    1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 8, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1,
    3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 8, 6, 6, 1, 6,
    2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1, 6, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2,
    2, 1, 6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2,
    1, 4, 3, 3, 1, 3, 2, 2, 1, 8, 7, 7, 1, 7, 2, 2, 1, 7, 3, 3, 1, 3, 2, 2, 1,
    7, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 7, 5, 5, 1, 5, 2, 2, 1, 5,
    3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 7, 6,
    6, 1, 6, 2, 2, 1, 6, 3, 3, 1, 3, 2, 2, 1, 6, 4, 4, 1, 4, 2, 2, 1, 4, 3, 3,
    1, 3, 2, 2, 1, 6, 5, 5, 1, 5, 2, 2, 1, 5, 3, 3, 1, 3, 2, 2, 1, 5, 4, 4, 1,
    4, 2, 2, 1, 4, 3, 3, 1, 3, 2, 2, 1, 8, 8, 8, 8, 8, 8, 8, 2, 8, 8, 8, 3, 8,
    3, 3, 2, 8, 8, 8, 4, 8, 4, 4, 2, 8, 4, 4, 3, 4, 3, 3, 2, 8, 8, 8, 5, 8, 5,
    5, 2, 8, 5, 5, 3, 5, 3, 3, 2, 8, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3,
    2, 8, 8, 8, 6, 8, 6, 6, 2, 8, 6, 6, 3, 6, 3, 3, 2, 8, 6, 6, 4, 6, 4, 4, 2,
    6, 4, 4, 3, 4, 3, 3, 2, 8, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3, 3, 2, 6,
    5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2, 8, 8, 8, 7, 8, 7, 7, 2, 8, 7,
    7, 3, 7, 3, 3, 2, 8, 7, 7, 4, 7, 4, 4, 2, 7, 4, 4, 3, 4, 3, 3, 2, 8, 7, 7,
    5, 7, 5, 5, 2, 7, 5, 5, 3, 5, 3, 3, 2, 7, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3,
    4, 3, 3, 2, 8, 7, 7, 6, 7, 6, 6, 2, 7, 6, 6, 3, 6, 3, 3, 2, 7, 6, 6, 4, 6,
    4, 4, 2, 6, 4, 4, 3, 4, 3, 3, 2, 7, 6, 6, 5, 6, 5, 5, 2, 6, 5, 5, 3, 5, 3,
    3, 2, 6, 5, 5, 4, 5, 4, 4, 2, 5, 4, 4, 3, 4, 3, 3, 2, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 3, 8, 8, 8, 8, 8, 8, 8, 4, 8, 8, 8, 4, 8, 4, 4, 3,
    8, 8, 8, 8, 8, 8, 8, 5, 8, 8, 8, 5, 8, 5, 5, 3, 8, 8, 8, 5, 8, 5, 5, 4, 8,
    5, 5, 4, 5, 4, 4, 3, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 6, 8, 6, 6, 3, 8, 8,
    8, 6, 8, 6, 6, 4, 8, 6, 6, 4, 6, 4, 4, 3, 8, 8, 8, 6, 8, 6, 6, 5, 8, 6, 6,
    5, 6, 5, 5, 3, 8, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 8, 8, 8, 8,
    8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 3, 8, 8, 8, 7, 8, 7, 7, 4, 8, 7, 7, 4, 7,
    4, 4, 3, 8, 8, 8, 7, 8, 7, 7, 5, 8, 7, 7, 5, 7, 5, 5, 3, 8, 7, 7, 5, 7, 5,
    5, 4, 7, 5, 5, 4, 5, 4, 4, 3, 8, 8, 8, 7, 8, 7, 7, 6, 8, 7, 7, 6, 7, 6, 6,
    3, 8, 7, 7, 6, 7, 6, 6, 4, 7, 6, 6, 4, 6, 4, 4, 3, 8, 7, 7, 6, 7, 6, 6, 5,
    7, 6, 6, 5, 6, 5, 5, 3, 7, 6, 6, 5, 6, 5, 5, 4, 6, 5, 5, 4, 5, 4, 4, 3, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 5, 8, 8, 8,
    8, 8, 8, 8, 5, 8, 8, 8, 5, 8, 5, 5, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 6, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 6, 8, 6, 6, 4, 8, 8, 8, 8, 8,
    8, 8, 6, 8, 8, 8, 6, 8, 6, 6, 5, 8, 8, 8, 6, 8, 6, 6, 5, 8, 6, 6, 5, 6, 5,
    5, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8,
    7, 8, 8, 8, 7, 8, 7, 7, 4, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 5,
    8, 8, 8, 7, 8, 7, 7, 5, 8, 7, 7, 5, 7, 5, 5, 4, 8, 8, 8, 8, 8, 8, 8, 7, 8,
    8, 8, 7, 8, 7, 7, 6, 8, 8, 8, 7, 8, 7, 7, 6, 8, 7, 7, 6, 7, 6, 6, 4, 8, 8,
    8, 7, 8, 7, 7, 6, 8, 7, 7, 6, 7, 6, 6, 5, 8, 7, 7, 6, 7, 6, 6, 5, 7, 6, 6,
    5, 6, 5, 5, 4, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 5, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 8, 8, 8, 8, 6,
    8, 8, 8, 6, 8, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 5, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7,
    8, 7, 7, 6, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 6, 8, 8, 8, 7, 8,
    7, 7, 6, 8, 7, 7, 6, 7, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    7, 8, 8, 8, 8, 8, 8, 8, 7, 8, 8, 8, 7, 8, 7, 7, 6, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 7};

// From https://github.com/ot/succinct/blob/master/broadword.hpp
static const uint64_t ones_step_4 = 0x1111111111111111ULL;
static const uint64_t ones_step_8 = 0x0101010101010101ULL;
static const uint64_t msbs_step_8 = 0x80ULL * ones_step_8;

// From https://github.com/ot/succinct/blob/master/broadword.hpp
inline uint64_t byte_counts(uint64_t x) {
    x = x - ((x & 0xa * ones_step_4) >> 1);
    x = (x & 3 * ones_step_4) + ((x >> 2) & 3 * ones_step_4);
    x = (x + (x >> 4)) & 0x0f * ones_step_8;
    return x;
}

#ifdef __AVX512VL__
// Compute prefix sum for 4 words in parallel, e.g., x=(2,3,1,2) -> x=(2,5,6,8)
inline __m256i prefixsum_epi64(__m256i x) {
    x = _mm256_add_epi64(
        x, _mm256_maskz_permutex_epi64(0b11111110, x, _MM_SHUFFLE(2, 1, 0, 3)));
    x = _mm256_add_epi64(
        x, _mm256_maskz_permutex_epi64(0b11111100, x, _MM_SHUFFLE(1, 0, 3, 2)));
    return x;
}
#endif

// Modes of rank algorithms
enum class rank_modes {
    NOCPU,
#ifdef __SSE4_2__
    SSE4_2_POPCNT,
#endif
#ifdef __AVX2__
    AVX2_POPCNT,
#endif
#ifdef __AVX512VPOPCNTDQ__
    AVX512_POPCNT,
#endif
};

static const std::map<rank_modes, std::string> rank_mode_map = {
    {rank_modes::NOCPU, "NOCPU"},  //
#ifdef __SSE4_2__
    {rank_modes::SSE4_2_POPCNT, "SSE4_2_POPCNT"},  //
#endif
#ifdef __AVX2__
    {rank_modes::AVX2_POPCNT, "AVX2_POPCNT"},  //
#endif
#ifdef __AVX512VPOPCNTDQ__
    {rank_modes::AVX512_POPCNT, "AVX512_POPCNT"},  //
#endif
};

inline std::string print_rank_mode(rank_modes mode) {
    return rank_mode_map.find(mode)->second;
}

template <rank_modes>
inline uint64_t rank_u64(uint64_t) {
    assert(false);  // should not come
    return UINT64_MAX;
}
template <>
inline uint64_t rank_u64<rank_modes::NOCPU>(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    x = (0x0101010101010101ULL * x >> 56);
    return x;
}
#ifdef __SSE4_2__
template <>
inline uint64_t rank_u64<rank_modes::SSE4_2_POPCNT>(uint64_t x) {
    return static_cast<uint64_t>(_mm_popcnt_u64(x));
}
#endif
#ifdef __AVX2__
// https://github.com/WojciechMula/sse-popcount/blob/master/popcnt-avx2-harley-seal.cpp
__m256i popcount_m256i(const __m256i v) {
    static const __m256i m1 = _mm256_set1_epi8(0x55);
    static const __m256i m2 = _mm256_set1_epi8(0x33);
    static const __m256i m4 = _mm256_set1_epi8(0x0F);
    const __m256i t1 = _mm256_sub_epi8(v, (_mm256_srli_epi16(v, 1) & m1));
    const __m256i t2 =
        _mm256_add_epi8(t1 & m2, (_mm256_srli_epi16(t1, 2) & m2));
    const __m256i t3 = _mm256_add_epi8(t2, _mm256_srli_epi16(t2, 4)) & m4;
    return _mm256_sad_epu8(t3, _mm256_setzero_si256());
}
#endif

template <rank_modes Mode>
inline uint64_t rank_u256(const uint64_t* x, uint64_t i) {
    assert(i < 256);
    uint64_t block = i / 64;
    uint64_t offset = (i + 1) & 63;
    uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    uint64_t rank_in_block = rank_u64<Mode>(x[block] & mask);
    uint64_t sum = 0;
    if (block) {
        for (uint64_t b = 0; b <= block - 1; ++b) sum += rank_u64<Mode>(x[b]);
    }
    return sum + rank_in_block;
}
#ifdef __AVX2__
template <>
inline uint64_t rank_u256<rank_modes::AVX2_POPCNT>(const uint64_t* x,
                                                   uint64_t i) {
    assert(i < 256);
    uint64_t block = i / 64;
    uint64_t offset = (i + 1) & 63;
    uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    if (block == 0) {
        return rank_u64<rank_modes::SSE4_2_POPCNT>(x[0] & mask);
    } else if (block == 1) {
        const __m256i mx =
            popcount_m256i(_mm256_set_epi64x(0, 0, x[1] & mask, x[0]));
        return static_cast<uint64_t>(_mm256_extract_epi64(mx, 0) +
                                     _mm256_extract_epi64(mx, 1));
    } else if (block == 2) {
        const __m256i mx =
            popcount_m256i(_mm256_set_epi64x(0, x[2] & mask, x[1], x[0]));
        return static_cast<uint64_t>(_mm256_extract_epi64(mx, 0) +
                                     _mm256_extract_epi64(mx, 1) +
                                     _mm256_extract_epi64(mx, 2));
    } else {
        const __m256i mx =
            popcount_m256i(_mm256_set_epi64x(x[3] & mask, x[2], x[1], x[0]));
        return static_cast<uint64_t>(
            _mm256_extract_epi64(mx, 0) + _mm256_extract_epi64(mx, 1) +
            _mm256_extract_epi64(mx, 2) + _mm256_extract_epi64(mx, 3));
    }
}
#endif
#ifdef __AVX512VPOPCNTDQ__
template <>
inline uint64_t rank_u256<rank_modes::AVX512_POPCNT>(const uint64_t* x,
                                                     uint64_t i) {
    assert(i < 256);
    uint64_t block = i / 64;
    uint64_t offset = (i + 1) & 63;
    uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
    if (block == 0) {
        return rank_u64<rank_modes::SSE4_2_POPCNT>(x[0] & mask);
    } else if (block == 1) {
        const __m256i mx = _mm256_set_epi64x(0, 0, x[1] & mask, x[0]);
        return static_cast<uint64_t>(_mm256_extract_epi64(mx, 0) +
                                     _mm256_extract_epi64(mx, 1));
    } else if (block == 2) {
        const __m256i mx = _mm256_set_epi64x(0, x[2] & mask, x[1], x[0]);
        return static_cast<uint64_t>(_mm256_extract_epi64(mx, 0) +
                                     _mm256_extract_epi64(mx, 1) +
                                     _mm256_extract_epi64(mx, 2));
    } else {
        const __m256i mx = _mm256_set_epi64x(x[3] & mask, x[2], x[1], x[0]);
        return static_cast<uint64_t>(
            _mm256_extract_epi64(mx, 0) + _mm256_extract_epi64(mx, 1) +
            _mm256_extract_epi64(mx, 2) + _mm256_extract_epi64(mx, 3));
    }
}
#endif

// Modes of select algorithms
enum class select_modes {
    NOCPU_SDSL,
    NOCPU_BROADWORD,
#ifdef __SSE4_2__
    BMI1_TZCNT_SDSL,
    SSE4_2_POPCNT_BROADWORD,
#endif
#ifdef __BMI2__
    BMI2_PDEP_TZCNT,
#endif
#ifdef __AVX2__
    AVX2_POPCNT,
#endif
#ifdef __AVX512VL__
    AVX2_POPCNT_AVX512_PREFIX_SUM,
#endif
#ifdef __AVX512VPOPCNTDQ__
    AVX512_POPCNT,
    AVX512_POPCNT_EX,
#endif
};

static const std::map<select_modes, std::string> select_mode_map = {
    {select_modes::NOCPU_SDSL, "NOCPU_SDSL"},            //
    {select_modes::NOCPU_BROADWORD, "NOCPU_BROADWORD"},  //
#ifdef __SSE4_2__
    {select_modes::BMI1_TZCNT_SDSL, "BMI1_TZCNT_SDSL"},                  //
    {select_modes::SSE4_2_POPCNT_BROADWORD, "SSE4_2_POPCNT_BROADWORD"},  //
#endif
#ifdef __BMI2__
    {select_modes::BMI2_PDEP_TZCNT, "BMI2_PDEP_TZCNT"},  //
#endif
#ifdef __AVX2__
    {select_modes::AVX2_POPCNT, "AVX2_POPCNT"},  //
#endif
#ifdef __AVX512VL__
    {select_modes::AVX2_POPCNT_AVX512_PREFIX_SUM,
     "AVX2_POPCNT_AVX512_PREFIX_SUM"},  //
#endif
#ifdef __AVX512VPOPCNTDQ__
    {select_modes::AVX512_POPCNT, "AVX512_POPCNT"},        //
    {select_modes::AVX512_POPCNT_EX, "AVX512_POPCNT_EX"},  //
#endif
};

inline std::string print_select_mode(select_modes mode) {
    return select_mode_map.find(mode)->second;
}

template <select_modes>
inline uint64_t select_u64(uint64_t, uint64_t) {
    assert(false);  // should not come
    return UINT64_MAX;
}

// From https://github.com/simongog/sdsl-lite/blob/master/include/sdsl/bits.hpp
template <>
inline uint64_t select_u64<select_modes::NOCPU_SDSL>(uint64_t x, uint64_t i) {
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
inline uint64_t select_u64<select_modes::NOCPU_BROADWORD>(uint64_t x,
                                                          uint64_t k) {
    assert(k < rank_u64<rank_modes::SSE4_2_POPCNT>(x));

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
inline uint64_t select_u64<select_modes::BMI1_TZCNT_SDSL>(uint64_t x,
                                                          uint64_t i) {
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
inline uint64_t select_u64<select_modes::SSE4_2_POPCNT_BROADWORD>(uint64_t x,
                                                                  uint64_t k) {
    assert(k < rank_u64<rank_modes::SSE4_2_POPCNT>(x));

    uint64_t byte_sums = byte_counts(x) * ones_step_8;
    const uint64_t k_step_8 = k * ones_step_8;
    const uint64_t geq_k_step_8 =
        (((k_step_8 | msbs_step_8) - byte_sums) & msbs_step_8);
    const uint64_t place =
        rank_u64<rank_modes::SSE4_2_POPCNT>(geq_k_step_8) * 8;
    const uint64_t byte_rank =
        k - (((byte_sums << 8) >> place) & uint64_t(0xFF));
    return place + select_in_byte[((x >> place) & 0xFF) | (byte_rank << 8)];
}
#endif
#ifdef __BMI2__
template <>
inline uint64_t select_u64<select_modes::BMI2_PDEP_TZCNT>(uint64_t x,
                                                          uint64_t i) {
    return _tzcnt_u64(_pdep_u64(1ull << i, x));
}
#endif

// Without AVX512VPOPCNTDQ
template <select_modes Mode>
inline uint64_t select_u256(const uint64_t* x, uint64_t k) {
    assert(k < rank_u256<rank_modes::NOCPU>(x, 255));

#ifdef __SSE__
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);
#endif

    uint64_t i = 0, cnt = 0;
    while (i < 4) {
        if constexpr (Mode == select_modes::NOCPU_SDSL ||
                      Mode == select_modes::NOCPU_BROADWORD) {
            cnt = rank_u64<rank_modes::NOCPU>(x[i]);
        } else {
            cnt = rank_u64<rank_modes::SSE4_2_POPCNT>(x[i]);
        }
        if (k < cnt) { break; }
        k -= cnt;
        i++;
    }
    assert(k < cnt);

    return i * 64 + select_u64<Mode>(x[i], k);
}

#ifdef __AVX2__
template <>
inline uint64_t select_u256<select_modes::AVX2_POPCNT>(const uint64_t* x,
                                                       uint64_t k) {
    assert(k < rank_u256<rank_modes::SSE4_2_POPCNT>(x, 255));
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);

    const __m256i mx =
        popcount_m256i(_mm256_set_epi64x(x[3], x[2], x[1], x[0]));
    const uint64_t cnts[4] = {
        static_cast<uint64_t>(_mm256_extract_epi64(mx, 0)),
        static_cast<uint64_t>(_mm256_extract_epi64(mx, 1)),
        static_cast<uint64_t>(_mm256_extract_epi64(mx, 2)),
        static_cast<uint64_t>(_mm256_extract_epi64(mx, 3))};

    uint64_t i = 0;
    while (i < 4) {
        if (k < cnts[i]) { break; }
        k -= cnts[i];
        i++;
    }
    assert(k < cnts[i]);

    return i * 64 + select_u64<select_modes::BMI2_PDEP_TZCNT>(x[i], k);
}
#endif

#ifdef __AVX512VL__
template <>
inline uint64_t select_u256<select_modes::AVX2_POPCNT_AVX512_PREFIX_SUM>(
    const uint64_t* x, uint64_t k) {
    assert(k < rank_u256<rank_modes::SSE4_2_POPCNT>(x, 255));
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);

    const __m256i mx =
        popcount_m256i(_mm256_set_epi64x(x[3], x[2], x[1], x[0]));
    const __m256i mc = prefixsum_epi64(mx);

    const __m256i mk = _mm256_set_epi64x(k, k, k, k);
    const __mmask8 mask = _mm256_cmp_epi64_mask(mc, mk, 2);  // 1 if mc <= mk
    const uint8_t i = lt_cnt[mask];

    const uint64_t sums[5] = {
        0ULL,  // Sentinel
        static_cast<uint64_t>(_mm256_extract_epi64(mc, 0)),
        static_cast<uint64_t>(_mm256_extract_epi64(mc, 1)),
        static_cast<uint64_t>(_mm256_extract_epi64(mc, 2)),
        static_cast<uint64_t>(_mm256_extract_epi64(mc, 3))};

    return i * 64 +
           select_u64<select_modes::BMI2_PDEP_TZCNT>(x[i], k - sums[i]);
}
#endif

#ifdef __AVX512VPOPCNTDQ__
template <>
inline uint64_t select_u256<select_modes::AVX512_POPCNT>(const uint64_t* x,
                                                         uint64_t k) {
    assert(k < rank_u256<rank_modes::SSE4_2_POPCNT>(x, 255));
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);

    const __m256i mx =
        _mm256_popcnt_epi64(_mm256_set_epi64x(x[3], x[2], x[1], x[0]));
    const uint64_t cnts[4] = {
        static_cast<uint64_t>(_mm256_extract_epi64(mx, 0)),
        static_cast<uint64_t>(_mm256_extract_epi64(mx, 1)),
        static_cast<uint64_t>(_mm256_extract_epi64(mx, 2)),
        static_cast<uint64_t>(_mm256_extract_epi64(mx, 3))};

    uint64_t i = 0;
    while (i < 4) {
        if (k < cnts[i]) { break; }
        k -= cnts[i];
        i++;
    }
    assert(k < cnts[i]);

    return i * 64 + select_u64<select_modes::BMI2_PDEP_TZCNT>(x[i], k);
}
template <>
inline uint64_t select_u256<select_modes::AVX512_POPCNT_EX>(const uint64_t* x,
                                                            uint64_t k) {
    assert(k < rank_u256<rank_modes::SSE4_2_POPCNT>(x, 255));
    _mm_prefetch(reinterpret_cast<const char*>(x), _MM_HINT_T0);

    const __m256i mx =
        _mm256_popcnt_epi64(_mm256_set_epi64x(x[3], x[2], x[1], x[0]));
    const __m256i mc = prefixsum_epi64(mx);

    const __m256i mk = _mm256_set_epi64x(k, k, k, k);
    const __mmask8 mask = _mm256_cmp_epi64_mask(mc, mk, 2);  // 1 if mc <= mk
    const uint8_t i = lt_cnt[mask];

    const uint64_t sums[5] = {
        0ULL,  // Sentinel
        static_cast<uint64_t>(_mm256_extract_epi64(mc, 0)),
        static_cast<uint64_t>(_mm256_extract_epi64(mc, 1)),
        static_cast<uint64_t>(_mm256_extract_epi64(mc, 2)),
        static_cast<uint64_t>(_mm256_extract_epi64(mc, 3))};

    return i * 64 +
           select_u64<select_modes::BMI2_PDEP_TZCNT>(x[i], k - sums[i]);
}
#endif

}  // namespace dyrs
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

/* * * * * * * * * * * * * * * * * *
 *
 *   Popcount implementations
 *
 * * * * * * * * * * * * * * * * */
enum class popcount_modes : int {
    broadword = 0x00,
    builtin = 0x01,
    avx2 = 0x02,
};
template <popcount_modes>
inline uint64_t popcount_u64(uint64_t) {
    assert(false);
    return 0;  // should not come
}
template <>
inline uint64_t popcount_u64<popcount_modes::broadword>(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    x = (0x0101010101010101ULL * x >> 56);
    return x;
}
#ifdef __SSE4_2__
template <>
inline uint64_t popcount_u64<popcount_modes::builtin>(uint64_t x) {
    return static_cast<uint64_t>(__builtin_popcountll(x));
}
#endif
#ifdef __AVX2__
// https://github.com/WojciechMula/sse-popcount/blob/master/popcnt-avx2-harley-seal.cpp
inline __m256i popcount_m256i(const __m256i v) {
    static const __m256i m1 = _mm256_set1_epi8(0x55);
    static const __m256i m2 = _mm256_set1_epi8(0x33);
    static const __m256i m4 = _mm256_set1_epi8(0x0F);
    const __m256i t1 = _mm256_sub_epi8(v, (_mm256_srli_epi16(v, 1) & m1));
    const __m256i t2 =
        _mm256_add_epi8(t1 & m2, (_mm256_srli_epi16(t1, 2) & m2));
    const __m256i t3 = _mm256_add_epi8(t2, _mm256_srli_epi16(t2, 4)) & m4;
    return _mm256_sad_epu8(t3, _mm256_setzero_si256());
}
// https://github.com/WojciechMula/sse-popcount/blob/master/popcnt-avx512-harley-seal.cpp
inline __m512i popcount_m512i(const __m512i v) {
    static const __m512i m1 = _mm512_set1_epi8(0x55);
    static const __m512i m2 = _mm512_set1_epi8(0x33);
    static const __m512i m4 = _mm512_set1_epi8(0x0F);
    const __m512i t1 = _mm512_sub_epi8(v, (_mm512_srli_epi16(v, 1) & m1));
    const __m512i t2 =
        _mm512_add_epi8(t1 & m2, (_mm512_srli_epi16(t1, 2) & m2));
    const __m512i t3 = _mm512_add_epi8(t2, _mm512_srli_epi16(t2, 4)) & m4;
    return _mm512_sad_epu8(t3, _mm512_setzero_si512());
}
#endif

/* * * * * * * * * * * * * * * * * *
 *
 *   PrefixSum implementations
 *
 * * * * * * * * * * * * * * * * */
enum class prefixsum_modes : int {
    loop = 0x00,
    unrolled = 0x01,
    parallel = 0x02,
};

#ifdef __AVX512VL__
// Compute prefix sums for 4 words in parallel, e.g., x=(2,3,1,2) -> x=(2,5,6,8)
inline __m256i prefixsum_m256i(__m256i x) {
    x = _mm256_add_epi64(
        x, _mm256_maskz_permutex_epi64(0b11111110, x, _MM_SHUFFLE(2, 1, 0, 3)));
    x = _mm256_add_epi64(
        x, _mm256_maskz_permutex_epi64(0b11111100, x, _MM_SHUFFLE(1, 0, 3, 2)));
    return x;
}
// Compute prefix sums for 8 words in parallel
inline __m512i prefixsum_m512i(__m512i x) {
    static const __m512i i1 = _mm512_set_epi64(6, 5, 4, 3, 2, 1, 0, 7);
    static const __m512i i2 = _mm512_set_epi64(5, 4, 3, 2, 1, 0, 7, 6);
    static const __m512i i3 = _mm512_set_epi64(3, 2, 1, 0, 7, 6, 5, 4);
    x = _mm512_add_epi64(x, _mm512_maskz_permutexvar_epi64(0b11111110, i1, x));
    x = _mm512_add_epi64(x, _mm512_maskz_permutexvar_epi64(0b11111100, i2, x));
    x = _mm512_add_epi64(x, _mm512_maskz_permutexvar_epi64(0b11110000, i3, x));
    return x;
}
#endif

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
    avx2_unrolled = int(popcount_modes::avx2) |  //
                    int(prefixsum_modes::unrolled) << 8,
#endif
#ifdef __AVX512VL__
    avx2_parallel = int(popcount_modes::avx2) |  //
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
    {rank_modes::avx2_unrolled, "avx2_unrolled"},  //
#endif
#ifdef __AVX512VL__
    {rank_modes::avx2_parallel, "avx2_parallel"},  //
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
#ifdef __AVX2__
template <>
inline uint64_t rank_u512<rank_modes::avx2_unrolled>(const uint64_t* x,
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
#endif
#ifdef __AVX512VL__
template <>
inline uint64_t rank_u512<rank_modes::avx2_parallel>(const uint64_t* x,
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
inline uint64_t select_u512<select_modes::avx2_avx512_pdep>(const uint64_t* x,
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
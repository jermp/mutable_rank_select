#pragma once

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../external/doctest/doctest/doctest.h"

#include <iostream>
#include <vector>

#include "../external/essentials/include/essentials.hpp"
#include "util.hpp"
#include "rs256/algorithms.hpp"
#include "types.hpp"

namespace dyrs::testing {
static constexpr uint32_t sizes[] = {
    5,       9,       13,      25,       37,       251,      316,     398,
    501,     630,     794,     1000,     1258,     1584,     1995,    2511,
    3162,    3981,    5011,    6309,     7943,     10000,    12589,   15848,
    19952,   25118,   31622,   39810,    50118,    63095,    79432,   100000,
    125892,  158489,  199526,  251188,   316227,   398107,   501187,  630957,
    794328,  1000000, 1258925, 1584893,  1995262,  2511886,  3162277, 3981071,
    5011872, 6309573, 7943282, 10000000, 12589254, 15848931,
    16777216  // max value
};
}

using namespace dyrs;
using namespace dyrs::testing;
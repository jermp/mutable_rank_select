#include <vector>
#include <iostream>
#include <array>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"

#include "util.hpp"
#include "rs256/algorithms.hpp"

using namespace dyrs;

static constexpr int runs = 100;
static constexpr uint32_t num_queries = 1000000;
static constexpr uint64_t bits_seed = 13;
static constexpr double size = 256;

enum class popcount_modes { BROADWORD, BUILTIN, AVX2 };

template <popcount_modes>
void popcount_256(const uint64_t*, volatile uint64_t*) {
    assert(false);  // should not come
}
template <>
void popcount_256<popcount_modes::BROADWORD>(const uint64_t* x,
                                             volatile uint64_t* tmp) {
    tmp[0] = rank_u64<rank_modes::NOCPU>(x[0]);
    tmp[1] = rank_u64<rank_modes::NOCPU>(x[1]);
    tmp[2] = rank_u64<rank_modes::NOCPU>(x[2]);
    tmp[3] = rank_u64<rank_modes::NOCPU>(x[3]);
}
template <>
void popcount_256<popcount_modes::BUILTIN>(const uint64_t* x,
                                           volatile uint64_t* tmp) {
    tmp[0] = rank_u64<rank_modes::SSE4_2_POPCNT>(x[0]);
    tmp[1] = rank_u64<rank_modes::SSE4_2_POPCNT>(x[1]);
    tmp[2] = rank_u64<rank_modes::SSE4_2_POPCNT>(x[2]);
    tmp[3] = rank_u64<rank_modes::SSE4_2_POPCNT>(x[3]);
}

template <popcount_modes>
void popcount_256(const __m256i x, volatile __m256i& tmp) {
    assert(false);  // should not come
}
template <>
void popcount_256<popcount_modes::AVX2>(const __m256i x,
                                        volatile __m256i& tmp) {
    tmp = popcount_m256i(x);
}

template <popcount_modes Mode>
void test(std::string type, const double density) {
    std::vector<uint64_t> bits(size / 64);
    auto num_ones = create_random_bits(bits, UINT64_MAX * density, bits_seed);
    std::cout << "# number of ones: " << num_ones << " (" << num_ones / size
              << ")" << std::endl;

    std::string json("{\"type\":\"" + type + "\", ");
    json += "\"density\":\"" + std::to_string(density) + "\", ";
    json += "\"timings\":[";

    essentials::timer_type t;
    double min = 0.0, max = 0.0, avg = 0.0;

    auto measure = [&]() {
        if constexpr (Mode != popcount_modes::AVX2) {
            volatile uint64_t tmp[4] = {};  // to avoid the optimization
            const uint64_t* x = bits.data();
            for (int run = 0; run != runs; run++) {
                t.start();
                for (uint64_t i = 0; i < num_queries; i++) {
                    popcount_256<Mode>(x, tmp);
                }
                t.stop();
            }
            std::cout << "# ignore: " << tmp[0] << tmp[1] << tmp[2] << tmp[3]
                      << std::endl;
        } else {
            volatile __m256i tmp;  // to avoid the optimization
            const __m256i x = _mm256_loadu_si256((__m256i const*)bits.data());
            for (int run = 0; run != runs; run++) {
                t.start();
                for (uint64_t i = 0; i < num_queries; i++) {
                    popcount_256<Mode>(x, tmp);
                }
                t.stop();
            }
            std::cout << "# ignore: "  //
                      << _mm256_extract_epi64(tmp, 0)
                      << _mm256_extract_epi64(tmp, 1)
                      << _mm256_extract_epi64(tmp, 2)
                      << _mm256_extract_epi64(tmp, 3) << std::endl;
        }
    };

    static constexpr int K = 10;

    // warm-up
    for (int k = 0; k != K; ++k) {
        measure();
        double avg_ns_query = (t.average() * 1000) / num_queries;
        avg += avg_ns_query;
        t.reset();
    }
    std::cout << "# warm-up: " << avg / K << std::endl;
    avg = 0.0;

    for (int k = 0; k != K; ++k) {
        measure();
        t.discard_max();
        double avg_ns_query = (t.max() * 1000) / num_queries;
        max += avg_ns_query;
        t.reset();
    }

    for (int k = 0; k != K; ++k) {
        measure();
        t.discard_min();
        t.discard_max();
        double avg_ns_query = (t.average() * 1000) / num_queries;
        avg += avg_ns_query;
        t.reset();
    }

    for (int k = 0; k != K; ++k) {
        measure();
        t.discard_min();
        double avg_ns_query = (t.min() * 1000) / num_queries;
        min += avg_ns_query;
        t.reset();
    }

    min /= K;
    max /= K;
    avg /= K;
    std::vector<double> tt{min, avg, max};
    std::sort(tt.begin(), tt.end());
    std::cout << "[" << tt[0] << "," << tt[1] << "," << tt[2] << "]\n";

    json += "[" + std::to_string(tt[0]) + "," + std::to_string(tt[1]) + "," +
            std::to_string(tt[2]) + "],";

    json.pop_back();
    json += "]}";
    std::cerr << json << std::endl;
}

int main(int argc, char** argv) {
    cmd_line_parser::parser parser(argc, argv);
    parser.add("mode", "Mode of rank algorithm.");
    parser.add("density", "Density of ones (in [0,1]).");
    if (!parser.parse()) return 1;

    auto mode = parser.get<std::string>("mode");
    auto density = parser.get<double>("density");

    if (mode == "broadword") {
        test<popcount_modes::BROADWORD>(mode, density);
    }
#ifdef __SSE4_2__
    else if (mode == "builtin") {
        test<popcount_modes::BUILTIN>(mode, density);
    }
#endif
#ifdef __AVX2__
    else if (mode == "avx2") {
        test<popcount_modes::AVX2>(mode, density);
    }
#endif
    else {
        std::cout << "unknown mode \"" << mode << "\"" << std::endl;
        return 1;
    }

    return 0;
}

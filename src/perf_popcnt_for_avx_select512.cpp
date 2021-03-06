#include <vector>
#include <iostream>
#include <array>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"

#include "util.hpp"
#include "rank_select_algorithms/rank.hpp"
#include "rank_select_algorithms/select.hpp"

using namespace mrs;

static constexpr int runs = 100;
static constexpr uint32_t num_queries = 1000000;
static constexpr uint64_t bits_seed = 13;
static constexpr uint64_t query_seed = 71;
static constexpr double density = 0.3;

static constexpr std::array<uint64_t, 1> sizes = {
    1ULL << 9,
};
// static constexpr std::array<uint64_t, 24> sizes = {
//     1ULL << 9,  1ULL << 10, 1ULL << 11, 1ULL << 12, 1ULL << 13, 1ULL << 14,
//     1ULL << 15, 1ULL << 16, 1ULL << 17, 1ULL << 18, 1ULL << 19, 1ULL << 20,
//     1ULL << 21, 1ULL << 22, 1ULL << 23, 1ULL << 24, 1ULL << 25, 1ULL << 26,
//     1ULL << 27, 1ULL << 28, 1ULL << 29, 1ULL << 30, 1ULL << 31, 1ULL << 32,
// };

template <popcount_modes>
inline __m512i popcount_512(const uint64_t*) {
    assert(false);  // should not come
    return __m512i{};
}
template <>
inline __m512i popcount_512<popcount_modes::builtin>(const uint64_t* x) {
    static uint64_t cnts[8];
    cnts[0] = popcount_u64<popcount_modes::builtin>(x[0]);
    cnts[1] = popcount_u64<popcount_modes::builtin>(x[1]);
    cnts[2] = popcount_u64<popcount_modes::builtin>(x[2]);
    cnts[3] = popcount_u64<popcount_modes::builtin>(x[3]);
    cnts[4] = popcount_u64<popcount_modes::builtin>(x[4]);
    cnts[5] = popcount_u64<popcount_modes::builtin>(x[5]);
    cnts[6] = popcount_u64<popcount_modes::builtin>(x[6]);
    cnts[7] = popcount_u64<popcount_modes::builtin>(x[7]);
    return _mm512_loadu_si512(reinterpret_cast<const __m512i*>(cnts));
}
#ifdef __AVX2__
template <>
inline __m512i popcount_512<popcount_modes::avx2>(const uint64_t* x) {
    const __m512i mx = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(x));
    return popcount_m512i(mx);
}
#endif

template <popcount_modes Mode>
void test(std::string type) {
    std::vector<uint64_t> bits(sizes.back() / 64);
    auto num_ones = create_random_bits(bits, UINT64_MAX * density, bits_seed);
    std::cout << "# number of ones: " << num_ones << " ("
              << num_ones / double(sizes.back()) << ")" << std::endl;

    std::string json("{\"type\":\"" + type + "\", ");
    json += "\"density\":\"" + std::to_string(density) + "\", ";
    json += "\"timings\":[";

    for (const uint64_t n : sizes) {
        splitmix64 hasher(query_seed);

        const auto num_buckets = n / 512;
        std::vector<const uint64_t*> queries(num_queries);

        for (uint64_t i = 0; i < num_queries; i++) {
            queries[i] = &bits[(hasher.next() % num_buckets) * 512 / 64];
        }

        essentials::timer_type t;
        double min = 0.0, max = 0.0, avg = 0.0;

        auto measure = [&]() {
            __m512i tmp{};  // to avoid the optimization
            for (int run = 0; run != runs; run++) {
                t.start();
                for (uint64_t i = 0; i < num_queries; i++) {
                    const uint64_t* x = queries[i];
                    tmp = _mm512_add_epi64(tmp, popcount_512<Mode>(x));
                }
                t.stop();
            }
            uint64_t tmp2[8];
            _mm512_storeu_si512(reinterpret_cast<__m512i*>(tmp2), tmp);
            std::cout << "# ignore: " << tmp2[0] << std::endl;
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

        json += "[" + std::to_string(tt[0]) + "," + std::to_string(tt[1]) +
                "," + std::to_string(tt[2]) + "],";
    }

    json.pop_back();
    json += "]}";
    std::cerr << json << std::endl;
}

int main(int argc, char** argv) {
    cmd_line_parser::parser parser(argc, argv);
    parser.add("mode", "Mode of popcnt algorithm.");
    if (!parser.parse()) return 1;

    auto mode = parser.get<std::string>("mode");

    if (mode == "builtin") {
        test<popcount_modes::builtin>(mode);
    }
#ifdef __AVX2__
    else if (mode == "avx2") {
        test<popcount_modes::avx2>(mode);
    }
#endif
    else {
        std::cout << "unknown mode \"" << mode << "\"" << std::endl;
        return 1;
    }
    return 0;
}

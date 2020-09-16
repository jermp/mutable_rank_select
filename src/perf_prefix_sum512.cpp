#include <vector>
#include <iostream>
#include <array>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"

#include "util.hpp"
#include "rank_select_algorithms/rank.hpp"
#include "rank_select_algorithms/select.hpp"

using namespace dyrs;

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

template <prefixsum_modes>
inline uint64_t prefixsum_512(const uint64_t*, uint64_t) {
    assert(false);  // should not come
    return UINT64_MAX;
}
template <>
inline uint64_t prefixsum_512<prefixsum_modes::loop>(const uint64_t* x,
                                                     uint64_t k) {
    uint64_t sum = 0;
    for (uint64_t i = 0; i <= k; i++) { sum += x[i]; }
    return sum;
}
template <>
inline uint64_t prefixsum_512<prefixsum_modes::unrolled>(const uint64_t* x,
                                                         uint64_t k) {
    static uint64_t sums[8] = {};
    sums[0] = x[0];
    sums[1] = sums[0] + x[1];
    sums[2] = sums[1] + x[2];
    sums[3] = sums[2] + x[3];
    sums[4] = sums[3] + x[4];
    sums[5] = sums[4] + x[5];
    sums[6] = sums[5] + x[6];
    sums[7] = sums[6] + x[7];
    return sums[k];
}
#ifdef __AVX512VL__
template <prefixsum_modes>
inline uint64_t prefixsum_512(__m512i, uint64_t) {
    assert(false);  // should not come
    return UINT64_MAX;
}
template <>
inline uint64_t prefixsum_512<prefixsum_modes::parallel>(__m512i x,
                                                         uint64_t k) {
    // volatile const __m512i y = prefixsum_m512i(x);
    // volatile const uint64_t* C = reinterpret_cast<volatile uint64_t
    // const*>(&y); return C[k];
    static uint64_t sums[8] = {};
    _mm512_storeu_si512((__m512i*)(sums), prefixsum_m512i(x));
    return sums[k];
}
#endif

template <prefixsum_modes Mode>
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
        std::vector<std::pair<const uint64_t*, uint64_t>> queries(num_queries);

        for (uint64_t i = 0; i < num_queries; i++) {
            const uint64_t* x = &bits[(hasher.next() % num_buckets) * 512 / 64];
            queries[i] = {x, hasher.next() % 8};
        }

        essentials::timer_type t;
        double min = 0.0, max = 0.0, avg = 0.0;

        auto measure = [&]() {
            uint64_t tmp = 0;  // to avoid the optimization
            if constexpr (Mode != prefixsum_modes::parallel) {
                for (int run = 0; run != runs; run++) {
                    t.start();
                    for (uint64_t i = 0; i < num_queries; i++) {
                        const uint64_t* x = queries[i].first;
                        const uint64_t k = queries[i].second;
                        tmp += prefixsum_512<Mode>(x, k);
                    }
                    t.stop();
                }
            } else {
                for (int run = 0; run != runs; run++) {
                    t.start();
                    for (uint64_t i = 0; i < num_queries; i++) {
                        const uint64_t* x = queries[i].first;
                        const uint64_t k = queries[i].second;
                        tmp += prefixsum_512<Mode>(
                            _mm512_loadu_si512((__m512i const*)x), k);
                    }
                    t.stop();
                }
            }
            std::cout << "# ignore: " << tmp << std::endl;
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
    parser.add("mode", "Mode of perfixsum algorithm.");
    if (!parser.parse()) return 1;

    auto mode = parser.get<std::string>("mode");

    if (mode == "loop") {
        test<prefixsum_modes::loop>(mode);
    } else if (mode == "unrolled") {
        test<prefixsum_modes::unrolled>(mode);
    }
#ifdef __AVX512VL__
    else if (mode == "parallel") {
        test<prefixsum_modes::parallel>(mode);
    }
#endif
    else {
        std::cout << "unknown mode \"" << mode << "\"" << std::endl;
        return 1;
    }

    return 0;
}

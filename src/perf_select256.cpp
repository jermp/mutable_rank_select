#include <vector>
#include <iostream>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"

#include "util.hpp"
#include "rs256/algorithms.hpp"

using namespace dyrs;

const uint64_t NUM_ITERS = 10;
const uint64_t NUM_QUERIES = 1'000'000;

template <select_modes Mode>
int main_template(
    const std::vector<std::pair<const uint64_t*, uint64_t>>& queries) {
    std::cout << "----- Algorithm: " << print_select_mode(Mode) << " -----"
              << std::endl;

    uint64_t tmp = 0;  // to avoid the optimization
    essentials::timer<essentials::clock_type, std::chrono::nanoseconds> t;

    for (size_t j = 0; j < NUM_ITERS; j++) {
        t.start();
        for (uint64_t i = 0; i < NUM_QUERIES; i++) {
            const uint64_t* x = queries[i].first;
            const uint64_t k = queries[i].second;
            tmp += select_u256<Mode>(x, k);
        }
        t.stop();
    }

    const double elapsed_ns = t.average();
    std::cout << "Computation time in ns/op: " << elapsed_ns / NUM_QUERIES
              << std::endl;
    std::cout << "tmp: " << tmp << std::endl;

    return 0;
}

int main(int argc, char** argv) {
#ifndef NDEBUG
    std::cerr << "The code is running in debug mode." << std::endl;
#endif

    cmd_line_parser::parser p(argc, argv);
    p.add("num_bits", "Number of bits (log_2)");
    p.add("density", "Density of ones");
    if (!p.parse()) { return 1; }

    const auto num_bits = std::max(256ULL, 1ULL << p.get<uint64_t>("num_bits"));
    const auto density = p.get<double>("density");

    std::vector<uint64_t> bits(num_bits / 64);
    const auto num_ones = create_random_bits(bits, UINT64_MAX * density);
    const auto num_buckets = num_bits / 256;

    std::cout << "Number of ones: " << num_ones << " ("
              << num_ones / double(num_bits) << ")" << std::endl;

    std::vector<std::pair<const uint64_t*, uint64_t>> queries(NUM_QUERIES);
    for (uint64_t i = 0; i < NUM_QUERIES;) {
        const uint64_t* x = &bits[(rand_u64() % num_buckets) * 256 / 64];
        const uint64_t r = rank_u256<rank_modes::NOCPU>(x, 255);
        if (r != 0) { queries[i++] = {x, rand_u64() % r}; }
    }

    main_template<select_modes::NOCPU_SDSL>(queries);
    main_template<select_modes::NOCPU_BROADWORD>(queries);
#ifdef __SSE4_2__
    main_template<select_modes::BMI1_TZCNT_SDSL>(queries);
    main_template<select_modes::SSE4_2_POPCNT_BROADWORD>(queries);
#endif
#ifdef __BMI2__
    main_template<select_modes::BMI2_PDEP_TZCNT>(queries);
#endif
#ifdef __AVX512VPOPCNTDQ__
    main_template<select_modes::AVX512_POPCNT>(queries);
    main_template<select_modes::AVX512_POPCNT_EX>(queries);
#endif

    return 0;
}

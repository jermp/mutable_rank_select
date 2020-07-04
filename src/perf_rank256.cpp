#include <vector>
#include <iostream>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"

#include "util.hpp"

using namespace dyrs;

const uint64_t NUM_ITERS = 10;
const uint64_t NUM_QUERIES = 1'000'000;

// From http://xoroshiro.di.unimi.it/splitmix64.c
uint64_t rand_u64() {
    static uint64_t x = 1;
    uint64_t z = (x += uint64_t(0x9E3779B97F4A7C15));
    z = (z ^ (z >> 30)) * uint64_t(0xBF58476D1CE4E5B9);
    z = (z ^ (z >> 27)) * uint64_t(0x94D049BB133111EB);
    return z ^ (z >> 31);
}

uint64_t create_random_bits(std::vector<uint64_t>& bits, uint64_t threshold) {
    uint64_t num_ones = 0;
    for (uint64_t i = 0; i < bits.size() * 64; i++) {
        if (rand_u64() < threshold) {
            bits[i / 64] |= 1ULL << (i % 64);
            num_ones++;
        }
    }
    return num_ones;
}

template <rank_modes Mode>
int main_template(const std::vector<const uint64_t*>& queries) {
    std::cout << "----- Algorithm: " << print_rank_mode(Mode) << " -----"
              << std::endl;

    uint64_t tmp = 0;  // to avoid the optimization
    essentials::timer<essentials::clock_type, std::chrono::nanoseconds> t;

    for (size_t j = 0; j < NUM_ITERS; j++) {
        t.start();
        for (uint64_t i = 0; i < NUM_QUERIES; i++) {
            tmp += rank_u256<Mode>(queries[i]);
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

    std::vector<const uint64_t*> queries(NUM_QUERIES);
    for (uint64_t i = 0; i < NUM_QUERIES; i++) {
        queries[i] = &bits[(rand_u64() % num_buckets) * 256 / 64];
    }

    main_template<rank_modes::NOCPU>(queries);
#ifdef __SSE4_2__
    main_template<rank_modes::SSE4_2_POPCNT>(queries);
#endif
#ifdef __AVX512VPOPCNTDQ__
    main_template<rank_modes::AVX512_POPCNT>(queries);
#endif

    return 0;
}

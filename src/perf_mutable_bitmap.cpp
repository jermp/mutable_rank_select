#include <iostream>
#include <iomanip>
#include <random>
#include <fstream>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"
#include "types.hpp"

using namespace mrs;

static constexpr int runs = 100;
static constexpr uint32_t num_queries = 10000;
static constexpr unsigned bits_seed = 13;
static constexpr unsigned query_seed = 71;

static constexpr uint64_t sizes[] = {
    1ULL << 9,  1ULL << 10, 1ULL << 11, 1ULL << 12, 1ULL << 13, 1ULL << 14,
    1ULL << 15, 1ULL << 16, 1ULL << 17, 1ULL << 18, 1ULL << 19, 1ULL << 20,
    1ULL << 21, 1ULL << 22, 1ULL << 23, 1ULL << 24, 1ULL << 25, 1ULL << 26,
    1ULL << 27, 1ULL << 28, 1ULL << 29, 1ULL << 30, 1ULL << 31, 1ULL << 32};

template <int I, template <uint32_t> typename SearchablePrefixSums,
          typename BlockType>
struct test {
    static void run(std::vector<uint64_t>& queries, std::string& json,
                    std::string const& operation, double density, int i) {
        if (i == -1 or i == I) {
            const uint64_t n = sizes[I];
            static constexpr uint64_t height = SearchablePrefixSums<1>::height(
                (n + BlockType::block_size - 1) / BlockType::block_size);
            mutable_bitmap<SearchablePrefixSums<height>, BlockType> vec;
            uint64_t num_ones = 0;
            {
                std::vector<uint64_t> bits((n + 63) / 64);
                num_ones =
                    create_random_bits(bits, UINT64_MAX * density, bits_seed);
                vec.build(bits.data(), bits.size());
            }

            double extra_space =
                ((vec.bytes() * 8.0 - vec.size()) * 100.0) / vec.size();
            std::cout << "### num_bits = " << n << "; extra_space "
                      << std::setprecision(3) << extra_space << "%"
                      << "; bits consumed per input bit "
                      << (vec.bytes() * 8.0) / vec.size() << std::endl;

            splitmix64 hasher(query_seed);
            uint64_t M = n;  // for rank and flip
            if (operation == "select") M = num_ones;
            std::generate(queries.begin(), queries.end(),
                          [&] { return hasher.next() % M; });

            essentials::timer_type t;
            double min = 0.0;
            double max = 0.0;
            double avg = 0.0;

            auto measure = [&]() {
                uint64_t total = 0;
                if (operation == "rank") {
                    for (int run = 0; run != runs; ++run) {
                        t.start();
                        for (auto q : queries) total += vec.rank(q);
                        t.stop();
                    }
                } else if (operation == "select") {
                    for (int run = 0; run != runs; ++run) {
                        t.start();
                        for (auto const& q : queries) total += vec.select(q);
                        t.stop();
                    }
                } else if (operation == "flip") {
                    for (int run = 0; run != runs; ++run) {
                        t.start();
                        for (auto q : queries) vec.flip(q);
                        t.stop();
                    }
                    total = vec.rank(vec.size() - 1);
                } else if (operation == "build") {
                    for (int run = 0; run != runs; ++run) {
                        t.start();
                        for (auto q : queries) total += q;
                        t.stop();
                    }
                } else {
                    assert(false);
                }
                std::cout << "# ignore: " << total << std::endl;
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

        test<I + 1, SearchablePrefixSums, BlockType>::run(
            queries, json, operation, density, i);
    }
};

template <template <uint32_t> typename SearchablePrefixSums, typename BlockType>
struct test<sizeof(sizes) / sizeof(sizes[0]), SearchablePrefixSums, BlockType> {
    static inline void run(std::vector<uint64_t>&, std::string&,
                           std::string const&, double, int) {}
};

template <template <uint32_t> typename SearchablePrefixSums, typename BlockType>
void perf_test(std::string const& operation, double density,
               std::string const& name, int i) {
    std::vector<uint64_t> queries(num_queries);
    auto str = mutable_bitmap<SearchablePrefixSums<1>, BlockType>::name();
    if (name != "") str = name;
    std::string json("{\"type\":\"" + str + "\", ");
    if (i != -1) json += "\"num_bits\":\"" + std::to_string(sizes[i]) + "\", ";
    json += "\"density\":\"" + std::to_string(density) + "\", ";
    json += "\"timings\":[";
    test<0, SearchablePrefixSums, BlockType>::run(queries, json, operation,
                                                  density, i);
    json.pop_back();
    json += "]}";
    std::cerr << json << std::endl;
}

int main(int argc, char** argv) {
    cmd_line_parser::parser parser(argc, argv);
    parser.add("type", "");
    parser.add(
        "operation",
        "Either 'rank', 'select', 'flip', or 'build'. If 'build' is "
        "specified, the data "
        "structure is only built and queries generated, but without running "
        "any query. Useful to compute the benchmark overhead, e.g., cache "
        "misses or cycles spent during these steps.");
    parser.add("density", "Density of ones (in [0,1]).");
    parser.add("name", "Friendly name to be logged.", "-n", false);
    parser.add("i",
               "Use a specific array size calculated as: 2^{9+i}. Running the "
               "program without this "
               "option will execute the benchmark for i = 0..23.",
               "-i", false);
    if (!parser.parse()) return 1;

    std::string name("");
    int i = -1;
    auto type = parser.get<std::string>("type");
    auto operation = parser.get<std::string>("operation");
    auto density = parser.get<double>("density");
    if (parser.parsed("name")) name = parser.get<std::string>("name");
    if (parser.parsed("i")) i = parser.get<int>("i");

    // if (type == "avx2_64") {
    //     perf_test<avx2::segment_tree, block64_type_default>(operation,
    //     density,
    //                                                         name, i);
    // } else if (type == "avx512_64") {
    //     perf_test<avx512::segment_tree, block64_type_default>(operation,
    //                                                           density, name,
    //                                                           i);
    // }

    if (type == "avx2_256_a") {
        perf_test<avx2::segment_tree, block256_type_a>(operation, density, name,
                                                       i);
    } else if (type == "avx512_256_a") {
        perf_test<avx512::segment_tree, block256_type_a>(operation, density,
                                                         name, i);
    }

    else if (type == "avx2_256_b") {
        perf_test<avx2::segment_tree, block256_type_b>(operation, density, name,
                                                       i);
    } else if (type == "avx512_256_b") {
        perf_test<avx512::segment_tree, block256_type_b>(operation, density,
                                                         name, i);
    }

    else if (type == "avx2_512_a") {
        perf_test<avx2::segment_tree, block512_type_a>(operation, density, name,
                                                       i);
    } else if (type == "avx512_512_a") {
        perf_test<avx512::segment_tree, block512_type_a>(operation, density,
                                                         name, i);
    }

    else if (type == "avx2_512_b") {
        perf_test<avx2::segment_tree, block512_type_b>(operation, density, name,
                                                       i);
    } else if (type == "avx512_512_b") {
        perf_test<avx512::segment_tree, block512_type_b>(operation, density,
                                                         name, i);
    }
#ifdef __AVX512VL__
    else if (type == "avx2_256_c") {
        perf_test<avx2::segment_tree, block256_type_c>(operation, density, name,
                                                       i);
    } else if (type == "avx512_256_c") {
        perf_test<avx512::segment_tree, block256_type_c>(operation, density,
                                                         name, i);
    }
#endif

    else {
        std::cout << "unknown type \"" << type << "\"" << std::endl;
        return 1;
    }

    return 0;
}

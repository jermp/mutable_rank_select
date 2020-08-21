#include <iostream>
#include <random>
#include <fstream>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"
#include "hft/fenwick.hpp"

static constexpr int runs = 100;
static constexpr uint32_t num_queries = 10000;
static constexpr unsigned value_seed = 13;
static constexpr unsigned query_seed = 71;

static constexpr uint32_t sizes[] = {
    1 << 8,  1 << 9,  1 << 10, 1 << 11, 1 << 12, 1 << 13,
    1 << 14, 1 << 15, 1 << 16, 1 << 17, 1 << 18, 1 << 19,
    1 << 20, 1 << 21, 1 << 22, 1 << 23, 1 << 24};

template <typename Tree>
void perf_test(std::string const& operation, std::string const& name) {
    essentials::uniform_int_rng<uint16_t> distr_values(0, 256, value_seed);
    std::vector<uint32_t> queries(num_queries);
    std::string json("{\"type\":\"" + name + "\", ");
    json += "\"timings\":[";
    for (int i = 0; i != sizeof(sizes) / sizeof(sizes[0]); ++i) {
        const uint32_t n = sizes[i];

        std::cout << "### n = " << n << "\n";

        std::vector<uint64_t  // as requested by hft classes
                    >
            input(n);
        std::generate(input.begin(), input.end(),
                      [&] { return distr_values.gen(); });
        Tree tree(input.data(), n);
        double bytes = static_cast<double>(tree.bitCount() + 7) / 8;
        // std::cout << "### space in bytes: " << bytes << std::endl;
        std::cout << "### space: " << bytes / n << " [bytes per element]"
                  << std::endl;

        if (operation == "sarch") {
            uint64_t max_sum = tree.prefix(n);
            essentials::uniform_int_rng<uint32_t> distr_queries(0, max_sum - 1,
                                                                query_seed);
            std::generate(queries.begin(), queries.end(),
                          [&] { return distr_queries.gen() + 1; });
        } else {
            essentials::uniform_int_rng<uint32_t> distr_queries(0, n - 1,
                                                                query_seed);
            std::generate(queries.begin(), queries.end(),
                          [&] { return distr_queries.gen() + 1; });
        }

        essentials::timer_type t;
        double min = 0.0;
        double max = 0.0;
        double avg = 0.0;

        auto measure = [&]() {
            uint64_t total = 0;
            if (operation == "sum") {
                for (int run = 0; run != runs; ++run) {
                    t.start();
                    for (auto q : queries) total += tree.prefix(q);
                    t.stop();
                }
            } else if (operation == "update") {
                static constexpr int8_t deltas[2] = {+1, -1};
                for (int run = 0; run != runs; ++run) {
                    t.start();
                    for (auto const& q : queries) {
                        tree.add(q, deltas[q & 1]);
                    }
                    t.stop();
                }
                total = tree.prefix(n);
            } else if (operation == "search") {
                for (int run = 0; run != runs; ++run) {
                    t.start();
                    for (auto q : queries) total += tree.find(q);
                    t.stop();
                }
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
    json.pop_back();
    json += "]}";
    std::cerr << json << std::endl;
}

int main(int argc, char** argv) {
    cmd_line_parser::parser parser(argc, argv);
    parser.add("type",
               "Searchable Prefix-Sum type. Available types are: 'ByteF(L)', "
               "'BitF(L)', 'FixedF(L)'.");
    parser.add(
        "operation",
        "Either 'sum', 'update', 'search', or 'build'. If 'build' is "
        "specified, the data "
        "structure is only built and queries generated, but without running "
        "sum or update. Useful to compute the benchmark overhead, e.g., cache "
        "misses or cycles spent during these steps.");
    if (!parser.parse()) return 1;

    std::string name("");
    auto type = parser.get<std::string>("type");
    auto operation = parser.get<std::string>("operation");

    constexpr uint64_t BOUND = 256;

    if (type == "ByteF") {
        perf_test<hft::fenwick::ByteF<BOUND>>(operation, type);
    } else if (type == "BitF") {
        perf_test<hft::fenwick::BitF<BOUND>>(operation, type);
    } else if (type == "FixedF") {
        perf_test<hft::fenwick::FixedF<BOUND>>(operation, type);
    } else if (type == "ByteL") {
        perf_test<hft::fenwick::ByteL<BOUND>>(operation, type);
    } else if (type == "BitL") {
        perf_test<hft::fenwick::BitL<BOUND>>(operation, type);
    } else if (type == "FixedL") {
        perf_test<hft::fenwick::FixedL<BOUND>>(operation, type);
    } else {
        std::cout << "unknown type \"" << type << "\"" << std::endl;
        return 1;
    }

    return 0;
}

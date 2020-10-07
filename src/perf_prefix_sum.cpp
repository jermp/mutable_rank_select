#include <iostream>
#include <random>
#include <fstream>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"
#include "types.hpp"

using namespace mrs;

static constexpr int runs = 100;
static constexpr uint32_t num_queries = 10000;
static constexpr unsigned value_seed = 13;
static constexpr unsigned query_seed = 71;

// static constexpr uint32_t sizes[] = {
//     251,      316,     398,     501,     630,     794,     1000,     1258,
//     1584,     1995,    2511,    3162,    3981,    5011,    6309,     7943,
//     10000,    12589,   15848,   19952,   25118,   31622,   39810,    50118,
//     63095,    79432,   100000,  125892,  158489,  199526,  251188,   316227,
//     398107,   501187,  630957,  794328,  1000000, 1258925, 1584893,  1995262,
//     2511886,  3162277, 3981071, 5011872, 6309573, 7943282, 10000000,
//     12589254, 15848931, 16777216  // max value
// };

static constexpr uint32_t sizes[] = {
    1 << 8,  1 << 9,  1 << 10, 1 << 11, 1 << 12, 1 << 13,
    1 << 14, 1 << 15, 1 << 16, 1 << 17, 1 << 18, 1 << 19,
    1 << 20, 1 << 21, 1 << 22, 1 << 23, 1 << 24};

template <int I, template <uint32_t> typename Tree>
struct test {
    static void run(essentials::uniform_int_rng<uint16_t>& distr_values,
                    std::vector<uint32_t>& queries, std::string& json,
                    std::string const& operation, int i) {
        if (i == -1 or i == I) {
            const uint32_t n = sizes[I];
            const uint32_t height = Tree<1>::height(n);
            Tree<height> tree;

            std::cout << "### n = " << n << "; height = " << height << "; "
                      << tree.name() << "\n";

            {
                std::vector<uint16_t> input(n);
                std::generate(input.begin(), input.end(),
                              [&] { return distr_values.gen(); });
                tree.build(input.data(), n);
            }
            // std::cout << "### space in bytes: " << tree.bytes() << std::endl;
            std::cout << "### space: " << static_cast<double>(tree.bytes()) / n
                      << " [bytes per element]" << std::endl;

            if (operation == "sarch") {
                uint64_t max_sum = tree.sum(n - 1);
                essentials::uniform_int_rng<uint32_t> distr_queries(
                    0, max_sum - 1, query_seed);
                std::generate(queries.begin(), queries.end(),
                              [&] { return distr_queries.gen(); });
            } else {
                essentials::uniform_int_rng<uint32_t> distr_queries(0, n - 1,
                                                                    query_seed);
                std::generate(queries.begin(), queries.end(),
                              [&] { return distr_queries.gen(); });
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
                        for (auto q : queries) total += tree.sum(q);
                        t.stop();
                    }
                } else if (operation == "update") {
                    static constexpr int8_t deltas[2] = {+1, -1};
                    for (int run = 0; run != runs; ++run) {
                        t.start();
                        for (auto const& q : queries) {
                            tree.update(q, deltas[q & 1]);
                        }
                        t.stop();
                    }
                    total = tree.sum(n - 1);
                } else if (operation == "search") {
                    for (int run = 0; run != runs; ++run) {
                        t.start();
                        for (auto q : queries) total += tree.search(q).position;
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

        test<I + 1, Tree>::run(distr_values, queries, json, operation, i);
    }
};

template <template <uint32_t> typename Tree>
struct test<sizeof(sizes) / sizeof(sizes[0]), Tree> {
    static inline void run(essentials::uniform_int_rng<uint16_t>&,
                           std::vector<uint32_t>&, std::string&,
                           std::string const&, int) {}
};

template <template <uint32_t> typename Tree>
void perf_test(std::string const& operation, std::string const& name, int i) {
    essentials::uniform_int_rng<uint16_t> distr_values(0, 256, value_seed);
    std::vector<uint32_t> queries(num_queries);
    auto str = Tree<1>::name();
    if (name != "") str = name;
    std::string json("{\"type\":\"" + str + "\", ");
    if (i != -1) json += "\"n\":\"" + std::to_string(sizes[i]) + "\", ";
    json += "\"timings\":[";

    test<0, Tree>::run(distr_values, queries, json, operation, i);

    json.pop_back();
    json += "]}";
    std::cerr << json << std::endl;
}

int main(int argc, char** argv) {
    cmd_line_parser::parser parser(argc, argv);
    parser.add("type",
               "Searchable Prefix-Sum type. Either 'avx2' or 'avx512'.");
    parser.add(
        "operation",
        "Either 'sum', 'update', 'search', or 'build'. If 'build' is "
        "specified, the data "
        "structure is only built and queries generated, but without running "
        "sum or update. Useful to compute the benchmark overhead, e.g., cache "
        "misses or cycles spent during these steps.");
    parser.add("name", "Friendly name to be logged.", "-n", false);
    parser.add("i",
               "Use a specific array size calculated as: floor(base^i) with "
               "base = 10^{1/10} = 1.25893. Running the program without this "
               "option will execute the benchmark for i = 24..72.",
               "-i", false);
    if (!parser.parse()) return 1;

    std::string name("");
    int i = -1;
    auto type = parser.get<std::string>("type");
    auto operation = parser.get<std::string>("operation");
    if (parser.parsed("name")) name = parser.get<std::string>("name");
    if (parser.parsed("i")) i = parser.get<int>("i");

    if (type == "avx2") {
        perf_test<avx2::segment_tree>(operation, name, i);
    } else if (type == "avx512") {
        perf_test<avx512::segment_tree>(operation, name, i);
    } else {
        std::cout << "unknown type \"" << type << "\"" << std::endl;
        return 1;
    }

    return 0;
}

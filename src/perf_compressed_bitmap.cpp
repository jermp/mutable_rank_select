/**
 *  Wrappers of Rank&Select library by Marcin Raniszewski
 *  Note that the implementation seems to handle bitmaps in big-endian.
 *  So, the results of rank/select would be different from our implementation,
 *  although it is not related to the performance comparison.
 *
 *  EXCEPTION REPORT:
 *  A segmentation fault occurred on rank_mpe(1|2|3) for N=2^27 and more :(
 */
#include <iostream>
#include <iomanip>
#include <random>
#include <fstream>
#include <memory>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"

#include "mranisz/shared/rank.hpp"
#include "mranisz/shared/select.hpp"

#include "util.hpp"

using namespace mrs;

static constexpr int runs = 100;
static constexpr uint32_t num_queries = 10000;
static constexpr unsigned bits_seed = 13;
static constexpr unsigned query_seed = 71;

static constexpr uint64_t sizes[] = {
    // 1ULL << 8,
    1ULL << 9,  1ULL << 10, 1ULL << 11, 1ULL << 12, 1ULL << 13, 1ULL << 14,
    1ULL << 15, 1ULL << 16, 1ULL << 17, 1ULL << 18, 1ULL << 19, 1ULL << 20,
    1ULL << 21, 1ULL << 22, 1ULL << 23, 1ULL << 24, 1ULL << 25, 1ULL << 26,
    1ULL << 27, 1ULL << 28, 1ULL << 29, 1ULL << 30, 1ULL << 31, 1ULL << 32};

enum class mranisz_rank_types {
    BASIC,
    BCH,
    CF,
    MPE1,
    MPE2,
    MPE3,
};
enum class mranisz_select_types {
    BASIC_128_4096,
    BASIC_512_8192,
    BCH_128_4096,
    BCH_512_8192,
    MPE1_128_4096,
    MPE2_128_4096,
    MPE3_128_4096,
};

template <mranisz_rank_types>
struct mranisz_rank_traits;

template <>
struct mranisz_rank_traits<mranisz_rank_types::BASIC> {
    using type = shared::RankBasic64<shared::RANK_BASIC_STANDARD>;
    static std::string name() {
        return "rank_basic";
    }
};
template <>
struct mranisz_rank_traits<mranisz_rank_types::BCH> {
    using type = shared::RankBasic64<shared::RANK_BASIC_COMPRESSED_HEADERS>;
    static std::string name() {
        return "rank_bch";
    }
};
template <>
struct mranisz_rank_traits<mranisz_rank_types::CF> {
    using type = shared::RankCF64;
    static std::string name() {
        return "rank_cf";
    }
};
template <>
struct mranisz_rank_traits<mranisz_rank_types::MPE1> {
    using type = shared::RankMPE64<shared::RANK_MPE1>;
    static std::string name() {
        return "rank_mpe1";
    }
};
template <>
struct mranisz_rank_traits<mranisz_rank_types::MPE2> {
    using type = shared::RankMPE64<shared::RANK_MPE2>;
    static std::string name() {
        return "rank_mpe2";
    }
};
template <>
struct mranisz_rank_traits<mranisz_rank_types::MPE3> {
    using type = shared::RankMPE64<shared::RANK_MPE3>;
    static std::string name() {
        return "rank_mpe3";
    }
};

template <mranisz_select_types>
struct mranisz_select_traits;

template <>
struct mranisz_select_traits<mranisz_select_types::BASIC_128_4096> {
    using type =
        shared::SelectBasic64<shared::SELECT_BASIC_STANDARD, 128, 4096>;
    static std::string name() {
        return "select_basic_128_4096";
    }
};
template <>
struct mranisz_select_traits<mranisz_select_types::BASIC_512_8192> {
    using type =
        shared::SelectBasic64<shared::SELECT_BASIC_STANDARD, 512, 8192>;
    static std::string name() {
        return "select_basic_512_8192";
    }
};
template <>
struct mranisz_select_traits<mranisz_select_types::BCH_128_4096> {
    using type = shared::SelectBasic64<shared::SELECT_BASIC_COMPRESSED_HEADERS,
                                       128, 4096>;
    static std::string name() {
        return "select_bch_128_4096";
    }
};
template <>
struct mranisz_select_traits<mranisz_select_types::BCH_512_8192> {
    using type = shared::SelectBasic64<shared::SELECT_BASIC_COMPRESSED_HEADERS,
                                       512, 8192>;
    static std::string name() {
        return "select_bch_512_8192";
    }
};
template <>
struct mranisz_select_traits<mranisz_select_types::MPE1_128_4096> {
    using type = shared::SelectMPE64<shared::SELECT_MPE1, 128, 4096>;
    static std::string name() {
        return "select_mpe1_128_4096";
    }
};
template <>
struct mranisz_select_traits<mranisz_select_types::MPE2_128_4096> {
    using type = shared::SelectMPE64<shared::SELECT_MPE2, 128, 4096>;
    static std::string name() {
        return "select_mpe2_128_4096";
    }
};
template <>
struct mranisz_select_traits<mranisz_select_types::MPE3_128_4096> {
    using type = shared::SelectMPE64<shared::SELECT_MPE3, 128, 4096>;
    static std::string name() {
        return "select_mpe3_128_4096";
    }
};

template <mranisz_rank_types T>
class mranisz_rank_wrapper {
public:
    using rank_type = typename mranisz_rank_traits<T>::type;

    mranisz_rank_wrapper() {}

    void build(std::vector<uint64_t>& bits) {
        uint8_t* text = reinterpret_cast<uint8_t*>(bits.data());
        uint64_t text_len = bits.size() * 8;

        m_index = std::make_unique<rank_type>();
        m_index->build(text, text_len);
    }

    static std::string name() {
        return mranisz_rank_traits<T>::name();
    }

    uint64_t operator()(uint64_t i) {
        return m_index->rank(i);
    }

    // Length of bitmap
    uint64_t size() {
        return m_index->getTextSize() * 8;
    }

    uint64_t bytes() {
        return m_index->getSize();
    }

private:
    std::unique_ptr<rank_type> m_index;
};

template <mranisz_select_types T>
class mranisz_select_wrapper {
public:
    using select_type = typename mranisz_select_traits<T>::type;

    mranisz_select_wrapper() {}

    void build(std::vector<uint64_t>& bits) {
        uint8_t* text = reinterpret_cast<uint8_t*>(bits.data());
        uint64_t text_len = bits.size() * 8;

        m_index = std::make_unique<select_type>();
        m_index->build(text, text_len);
    }

    static std::string name() {
        return mranisz_select_traits<T>::name();
    }

    uint64_t operator()(uint64_t i) {
        return m_index->select(i);
    }

    // Length of bitmap
    uint64_t size() {
        return m_index->getTextSize() * 8;
    }

    uint64_t bytes() {
        return m_index->getSize();
    }

private:
    std::unique_ptr<select_type> m_index;
};

template <int I, typename Wrapper>
struct test {
    static void run(std::vector<uint64_t>& queries, std::string& json,
                    std::string const& operation, double density, int i) {
        if (i == -1 or i == I) {
            Wrapper vec;

            const uint64_t n = sizes[I];
            uint64_t num_ones = 0;
            {
                // Following the original impl, we reserve one extra element
                // https://github.com/mranisz/rank-select/blob/master/shared/common.hpp#L138
                std::vector<uint64_t> bits;
                bits.reserve((n + 63) / 64 + 1);
                bits.resize((n + 63) / 64);
                num_ones =
                    create_random_bits(bits, UINT64_MAX * density, bits_seed);
                vec.build(bits);
            }

            std::cout << "### num_bits = " << n
                      << "; bits consumed per input bit "
                      << (vec.bytes() * 8.0) / n << std::endl;

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
                if (operation == "rank" || operation == "select") {
                    for (int run = 0; run != runs; ++run) {
                        t.start();
                        for (auto q : queries) total += vec(q);
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

        test<I + 1, Wrapper>::run(queries, json, operation, density, i);
    }
};

template <typename Wrapper>
struct test<sizeof(sizes) / sizeof(sizes[0]), Wrapper> {
    static inline void run(std::vector<uint64_t>&, std::string&,
                           std::string const&, double, int) {}
};

template <typename Wrapper>
void perf_test(std::string const& operation, double density,
               std::string const& name, int i) {
    std::vector<uint64_t> queries(num_queries);
    auto str = Wrapper::name();
    if (name != "") str = name;
    std::string json("{\"type\":\"" + str + "\", ");
    if (i != -1) json += "\"num_bits\":\"" + std::to_string(sizes[i]) + "\", ";
    json += "\"density\":\"" + std::to_string(density) + "\", ";
    json += "\"timings\":[";
    test<0, Wrapper>::run(queries, json, operation, density, i);
    json.pop_back();
    json += "]}";
    std::cerr << json << std::endl;
}

int main(int argc, char** argv) {
    cmd_line_parser::parser parser(argc, argv);
    parser.add("type", "Compressed Rank/Select data structure types.");
    parser.add(
        "operation",
        "Either 'rank', 'select', or 'build'. If 'build' is "
        "specified, the data "
        "structure is only built and queries generated, but without running "
        "any query. Useful to compute the benchmark overhead, e.g., cache "
        "misses or cycles spent during these steps. "
        "We assume that arguments 'rank' and 'select' are properly specified "
        "for the data structure.");
    parser.add("density", "Density of ones (in [0,1]).");
    parser.add("name", "Friendly name to be logged.", "-n", false);
    parser.add("i",
               "Use a specific array size calculated as: 2^{8+i}. Running the "
               "program without this "
               "option will execute the benchmark for i = 0..25.",
               "-i", false);
    if (!parser.parse()) return 1;

    std::string name("");
    int i = -1;
    auto type = parser.get<std::string>("type");
    auto operation = parser.get<std::string>("operation");
    auto density = parser.get<double>("density");
    if (parser.parsed("name")) name = parser.get<std::string>("name");
    if (parser.parsed("i")) i = parser.get<int>("i");

    if (type == "rank_basic") {
        using T = mranisz_rank_wrapper<mranisz_rank_types::BASIC>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "rank_bch") {
        using T = mranisz_rank_wrapper<mranisz_rank_types::BCH>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "rank_cf") {
        using T = mranisz_rank_wrapper<mranisz_rank_types::CF>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "rank_mpe1") {
        using T = mranisz_rank_wrapper<mranisz_rank_types::MPE1>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "rank_mpe2") {
        using T = mranisz_rank_wrapper<mranisz_rank_types::MPE2>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "rank_mpe3") {
        using T = mranisz_rank_wrapper<mranisz_rank_types::MPE3>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "select_basic_128_4096") {
        using T = mranisz_select_wrapper<mranisz_select_types::BASIC_128_4096>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "select_basic_512_8192") {
        using T = mranisz_select_wrapper<mranisz_select_types::BASIC_512_8192>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "select_bch_128_4096") {
        using T = mranisz_select_wrapper<mranisz_select_types::BCH_128_4096>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "select_bch_512_8192") {
        using T = mranisz_select_wrapper<mranisz_select_types::BCH_512_8192>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "select_mpe1_128_4096") {
        using T = mranisz_select_wrapper<mranisz_select_types::MPE1_128_4096>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "select_mpe2_128_4096") {
        using T = mranisz_select_wrapper<mranisz_select_types::MPE2_128_4096>;
        perf_test<T>(operation, density, name, i);
    } else if (type == "select_mpe3_128_4096") {
        using T = mranisz_select_wrapper<mranisz_select_types::MPE3_128_4096>;
        perf_test<T>(operation, density, name, i);
    } else {
        std::cout << "unknown type \"" << type << "\"" << std::endl;
        return 1;
    }

    return 0;
}

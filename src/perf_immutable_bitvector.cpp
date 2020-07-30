#include <iostream>
#include <iomanip>
#include <random>
#include <fstream>
#include <memory>

#include "../external/essentials/include/essentials.hpp"
#include "../external/cmd_line_parser/include/parser.hpp"

// SDSL
#include "../external/sdsl-lite/include/sdsl/rank_support.hpp"
#include "../external/sdsl-lite/include/sdsl/select_support.hpp"
#include "../external/sdsl-lite/include/sdsl/bit_vector_il.hpp"

// Succinct (or Broadword)
#include "../external/succinct/rs_bit_vector.hpp"
#include "../external/succinct/darray.hpp"
#include "../external/succinct/mapper.hpp"

// Poppy (with PTSelect)
#include "poppy/bitmap.h"

#include "util.hpp"

using namespace dyrs;

static constexpr int runs = 100;
static constexpr uint32_t num_queries = 10000;
static constexpr unsigned bits_seed = 13;
static constexpr unsigned query_seed = 71;

static constexpr uint64_t sizes[] = {
    1ULL << 8,  1ULL << 9,  1ULL << 10, 1ULL << 11, 1ULL << 12,
    1ULL << 13, 1ULL << 14, 1ULL << 15, 1ULL << 16, 1ULL << 17,
    1ULL << 18, 1ULL << 19, 1ULL << 20, 1ULL << 21, 1ULL << 22,
    1ULL << 23, 1ULL << 24, 1ULL << 25, 1ULL << 26, 1ULL << 27,
    1ULL << 28, 1ULL << 29, 1ULL << 30, 1ULL << 31, 1ULL << 32};

// SDSL implementation of Vigna’s Rank9, whose space overhead is 25%.
// c.f. "Broadword implementation of rank/select queries" (WEA2008) by Vigna.
using sdsl_rank_v = sdsl::rank_support_v<>;
// More space-efficient version of sdsl_rank_v whose space overhead is ~6%. c.f.
// Section 3 in "Optimized succinct data structures for massive data" (SPE2014)
// by Gog and Petri.
using sdsl_rank_v5 = sdsl::rank_support_v5<>;
// SDSL's simple implementation of Clark’s select data structure, whose space
// overhead is 12%. c.f. Section 4 in the same paper.
using sdsl_select_mcl = sdsl::select_support_mcl<>;

template <class RankSupport, class SelectSupport>
class sdsl_wrapper {
public:
    sdsl_wrapper() {}

    void build(std::vector<uint64_t> const& bits, bool enable_rank,
               bool enable_select) {
        m_bv = sdsl::bit_vector(bits.size() * 64);
        std::copy(bits.data(), bits.data() + bits.size(), m_bv.data());
        m_enable_rank = enable_rank;
        m_enable_select = enable_select;
        if (m_enable_rank) { sdsl::util::init_support(m_ranker, &m_bv); }
        if (m_enable_select) { sdsl::util::init_support(m_selecter, &m_bv); }
    }

    static std::string name() {
        std::string n = "sdsl";
        if constexpr (std::is_same_v<RankSupport, sdsl_rank_v>) { n += "_v"; }
        if constexpr (std::is_same_v<RankSupport, sdsl_rank_v5>) { n += "_v5"; }
        if constexpr (std::is_same_v<SelectSupport, sdsl_select_mcl>) {
            n += "_mcl";
        }
        return n;
    }

    uint64_t rank(uint64_t i) {
        assert(m_enable_rank);
        return m_ranker(i + 1);
    }

    uint64_t select(uint64_t i) {
        assert(m_enable_select);
        return m_selecter(i + 1);
    }

    uint64_t size() {
        return m_bv.size();
    }

    uint64_t bytes() {
        size_t sum = 0;
        sum += sdsl::size_in_bytes(m_bv);
        sum += m_enable_rank ? sdsl::size_in_bytes(m_ranker) : 0;
        sum += m_enable_select ? sdsl::size_in_bytes(m_selecter) : 0;
        return sum;
    }

private:
    sdsl::bit_vector m_bv;
    RankSupport m_ranker;
    SelectSupport m_selecter;
    bool m_enable_rank;
    bool m_enable_select;
};

using sdsl_v_mcl_wrapper = sdsl_wrapper<sdsl_rank_v, sdsl_select_mcl>;
using sdsl_v5_mcl_wrapper = sdsl_wrapper<sdsl_rank_v5, sdsl_select_mcl>;

// SDSL cache-friendly implementation of Vigna’s Rank9 using superblocks of size
// 512, whose space overhead is 13%. c.f. Section 3 in "Optimized succinct data
// structures for massive data" (SPE2014) by Gog and Petri. Select is performed
// by binary search for the rank dictionary.
class sdsl_il_wrapper {
public:
    sdsl_il_wrapper() {}

    void build(std::vector<uint64_t> const& bits, bool enable_rank,
               bool enable_select) {
        sdsl::bit_vector bv(bits.size() * 64);
        std::copy(bits.data(), bits.data() + bits.size(), bv.data());
        m_bv = sdsl::bit_vector_il<>(bv);
        m_enable_rank = enable_rank;
        m_enable_select = enable_select;
        if (m_enable_rank) { sdsl::util::init_support(m_ranker, &m_bv); }
        if (m_enable_select) { sdsl::util::init_support(m_selecter, &m_bv); }
    }

    static std::string name() {
        return "sdsl_il";
    }

    uint64_t rank(uint64_t i) {
        assert(m_enable_rank);
        return m_ranker(i + 1);
    }

    uint64_t select(uint64_t i) {
        assert(m_enable_select);
        return m_selecter(i + 1);
    }

    uint64_t size() {
        return m_bv.size();
    }

    uint64_t bytes() {
        size_t sum = 0;
        sum += sdsl::size_in_bytes(m_bv);
        sum += m_enable_rank ? sdsl::size_in_bytes(m_ranker) : 0;
        sum += m_enable_select ? sdsl::size_in_bytes(m_selecter) : 0;
        return sum;
    }

private:
    sdsl::bit_vector_il<> m_bv;
    sdsl::bit_vector_il<>::rank_1_type m_ranker;
    sdsl::bit_vector_il<>::select_1_type m_selecter;
    bool m_enable_rank;
    bool m_enable_select;
};

// Succinct implementation of Vigna’s Rank9, whose space overhead is 25%.
// c.f. "Broadword implementation of rank/select queries" (WEA2008) by Vigna.
// Select is performed by hinted bsearch, described in the same paper.
class succinct_broadword_wrapper {
public:
    succinct_broadword_wrapper() {}

    void build(std::vector<uint64_t> const& bits, bool, bool enable_select) {
        succinct::bit_vector_builder bvb;
        bvb.reserve(bits.size() * 64);
        for (const uint64_t& w : bits) { bvb.append_bits(w, 64); }
        succinct::rs_bit_vector(&bvb, enable_select, false).swap(m_bits);
    }

    static std::string name() {
        return "succinct_broadword";
    }

    uint64_t rank(uint64_t i) {
        return m_bits.rank(i + 1);
    }

    uint64_t select(uint64_t i) {
        return m_bits.select(i);
    }

    uint64_t size() {
        return m_bits.size();
    }

    uint64_t bytes() {
        return succinct::mapper::size_of(m_bits);
    }

private:
    succinct::rs_bit_vector m_bits;
};

// Succinct implementation of Select using Okanohara's darray, whose space
// overhead is 28%. c.f. "Practical entropy-compressed rank/select dictionary"
// (ALENEX2007) by Okanohara and Sadakane.
class succinct_darray_wrapper {
public:
    succinct_darray_wrapper() {}

    void build(std::vector<uint64_t> const& bits, bool enable_rank, bool) {
        if (enable_rank) {
            std::cout << "succinct_darray_wrapper does not support rank."
                      << std::endl;
            exit(1);
        }
        succinct::bit_vector_builder bvb;
        bvb.reserve(bits.size() * 64);
        for (const uint64_t& w : bits) { bvb.append_bits(w, 64); }
        succinct::bit_vector(&bvb).swap(m_bits);
        succinct::darray1(m_bits).swap(m_selecter);
    }

    static std::string name() {
        return "succinct_darray";
    }

    uint64_t rank(uint64_t) {
        return 0;
    }

    uint64_t select(uint64_t i) {
        return m_selecter.select(m_bits, i);
    }

    uint64_t size() {
        return m_bits.size();
    }

    uint64_t bytes() {
        return succinct::mapper::size_of(m_bits) +
               succinct::mapper::size_of(m_selecter);
    }

private:
    succinct::bit_vector m_bits;
    succinct::darray1 m_selecter;
};

// Author's implementation of Poppy, c.f. "Space-efficient, high-performance
// rank and select structures on uncompressed bit sequences" (SEA2013) by Zhou
// et al. Precisely, the implementation is based on PTSelect, described in "A
// Fast x86 Implementation of Select" by Pandey et al.
class poppy_wrapper {
public:
    poppy_wrapper() {}

    void build(std::vector<uint64_t> const& bits, bool, bool) {
        m_bits = bits;  // clone
        m_bitmap = std::unique_ptr<poppy::BitmapPoppy>(
            new poppy::BitmapPoppy(m_bits.data(), m_bits.size() * 64));
    }

    static std::string name() {
        return "poppy";
    }

    uint64_t rank(uint64_t i) {
        return m_bitmap->rank(i + 1);
    }

    uint64_t select(uint64_t i) {
        return m_bitmap->select(i + 1);
    }

    uint64_t size() {
        return m_bitmap->size();
    }

    uint64_t bytes() {
        return m_bits.size() * sizeof(uint64_t) + m_bitmap->bytes();
    }

private:
    std::vector<uint64_t> m_bits;
    std::unique_ptr<poppy::BitmapPoppy> m_bitmap;
};

template <int I, typename Wrapper>
struct test {
    static void run(std::vector<uint64_t>& queries, std::string& json,
                    std::string const& operation, double density, int i) {
        if (i == -1 or i == I) {
            const uint64_t n = sizes[I];
            Wrapper vec;
            uint64_t num_ones = 0;
            {
                std::vector<uint64_t> bits((n + 63) / 64);
                num_ones =
                    create_random_bits(bits, UINT64_MAX * density, bits_seed);
                vec.build(bits, operation == "rank", operation == "select");
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
    parser.add("type", "Immutable Rank/Select data structure types.");
    parser.add(
        "operation",
        "Either 'rank', 'select', or 'build'. If 'build' is "
        "specified, the data "
        "structure is only built and queries generated, but without running "
        "any query. Useful to compute the benchmark overhead, e.g., cache "
        "misses or cycles spent during these steps.");
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

    if (type == "sdsl_v_mcl") {
        perf_test<sdsl_v_mcl_wrapper>(operation, density, name, i);
    } else if (type == "sdsl_v5_mcl") {
        perf_test<sdsl_v5_mcl_wrapper>(operation, density, name, i);
    } else if (type == "sdsl_il") {
        perf_test<sdsl_il_wrapper>(operation, density, name, i);
    } else if (type == "succinct_broadword") {
        perf_test<succinct_broadword_wrapper>(operation, density, name, i);
    } else if (type == "succinct_darray") {
        perf_test<succinct_darray_wrapper>(operation, density, name, i);
    } else if (type == "poppy") {
        perf_test<poppy_wrapper>(operation, density, name, i);
    } else {
        std::cout << "unknown type \"" << type << "\"" << std::endl;
        return 1;
    }

    return 0;
}

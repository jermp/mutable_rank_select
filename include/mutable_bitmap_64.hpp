#pragma once

#include <vector>
#include "rank_select_algorithms/rank.hpp"
#include "rank_select_algorithms/select.hpp"

namespace dyrs {

/* Specialization of the class mutable_bitmap. In this case we use blocks of
 * 64 bits, instead of 256 bits. */
template <typename PrefixSums, rank_modes RankMode, select_modes SelectMode>
struct mutable_bitmap_64 {
    mutable_bitmap_64() {}

    void build(uint64_t const* in, uint64_t num_64bit_words) {
        assert(num_64bit_words * 64 <= (1ULL << 30));
        std::vector<uint16_t> prefix_sums_input;
        prefix_sums_input.reserve(num_64bit_words);
        m_bits.resize(num_64bit_words, 0);
        std::memcpy(m_bits.data(), in, num_64bit_words * sizeof(uint64_t));
        for (uint64_t i = 0; i != m_bits.size(); ++i) {
            uint16_t val = __builtin_popcountll(m_bits[i]);
            assert(val <= 64);
            prefix_sums_input.push_back(val);
        }
        m_index.build(prefix_sums_input.data(), prefix_sums_input.size());
    }

    static std::string name() {
        return "mutable_bitmap_64<" + PrefixSums::name() + "," +
               print_rank_mode(RankMode) + "," + print_select_mode(SelectMode) +
               ">";
    }

    uint64_t size() const {
        return m_bits.size() * sizeof(m_bits.front()) * 8;
    }

    uint64_t bytes() const {
        return m_index.bytes() + size() / 8;
    }

    auto const& bits() const {
        return m_bits;
    }

    inline bool at(uint64_t i) const {
        assert(i < size());
        uint64_t r;
        uint64_t w = m_bits[i / 64];
        __asm volatile(
            "bt %2,%1\n"
            "sbb %0,%0"
            : "=r"(r)
            : "r"(w), "r"(i));
        return r;
    }

    void flip(uint64_t i) {
        assert(i < size());
        bool bit = at(i);
        uint64_t word = i / 64;
        uint64_t offset = i & 63;
        m_bits[word] ^= 1ULL << offset;
        static constexpr int8_t deltas[2] = {+1, -1};
        m_index.update(word, deltas[bit]);
    }

    uint64_t rank(uint64_t i) const {
        assert(i < size());
        uint64_t word = i / 64;
        uint64_t offset = i & 63;
        uint64_t mask = (offset != 0) * (1ULL << offset) - 1;
        return popcount_u64<extract_popcount_mode(RankMode)>(m_bits[word] &
                                                             mask) +
               (word ? m_index.sum(word - 1) : 0);
    }

    uint64_t select(uint64_t i) const {
        assert(i < size());
        auto pair = m_index.search(i);
        uint64_t offset = i - pair.sum;  // i - m_index.sum(pair.position - 1);
        return 64 * pair.position +
               select_u64<extract_select64_mode(SelectMode)>(
                   m_bits[pair.position], offset);
    }

private:
    PrefixSums m_index;
    std::vector<uint64_t> m_bits;
};

}  // namespace dyrs
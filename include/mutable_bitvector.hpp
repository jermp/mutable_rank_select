#pragma once

#include <vector>
#include "rs256/algorithms.hpp"

namespace dyrs {

template <typename PrefixSums, rank_modes RankMode, select_modes SelectMode>
struct mutable_bitvector {
    mutable_bitvector() {}

    void build(uint64_t const* in, uint64_t num_64bit_words) {
        assert(num_64bit_words * 64 <= (1ULL << 32));
        uint64_t num_256bit_blocks =
            std::ceil(static_cast<double>(num_64bit_words) / 4);
        std::vector<uint16_t> prefix_sums_input;
        prefix_sums_input.reserve(num_256bit_blocks);
        m_bits.resize(4 * num_256bit_blocks, 0);  // pad to a multiple of 4
        std::memcpy(m_bits.data(), in, num_64bit_words * sizeof(uint64_t));
        for (uint64_t i = 0; i != m_bits.size(); i += 4) {
            uint16_t val = 0;
            for (uint64_t j = 0; j != 4; ++j) {
                val += __builtin_popcountll(m_bits[i + j]);
            }
            assert(val <= 256);
            prefix_sums_input.push_back(val);
        }
        m_index.build(prefix_sums_input.data(), prefix_sums_input.size());
    }

    static std::string name() {
        return "mutable_bitvector<" + PrefixSums::name() + "," +
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
        uint64_t block = i / 256;
        m_bits[word] ^= 1ULL << offset;
        static constexpr int8_t deltas[2] = {+1, -1};
        m_index.update(block, deltas[bit]);
    }

    uint64_t rank(uint64_t i) const {
        assert(i < size());
        uint64_t block = i / 256;
        uint64_t offset = i & 255;
        return rank_u256<RankMode>(&m_bits[4 * block], offset) +
               (block ? m_index.sum(block - 1) : 0);
    }

    uint64_t select(uint64_t i) const {
        assert(i < size());
        auto [block, sum] = m_index.search(i);
        uint64_t offset = i - sum;  // i - m_index.sum(block - 1);
        return 256 * block +
               select_u256<SelectMode>(&m_bits[4 * block], offset);
    }

private:
    PrefixSums m_index;
    std::vector<uint64_t> m_bits;
};

}  // namespace dyrs
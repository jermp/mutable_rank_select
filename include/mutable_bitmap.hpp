#pragma once

#include <vector>
#include "rank_select_algorithms/rank.hpp"
#include "rank_select_algorithms/select.hpp"

namespace dyrs {

template <typename SearchablePrefixSums, typename BlockType>
struct mutable_bitmap {
    // currently supported block sizes
    static_assert(BlockType::block_size == 64 or BlockType::block_size == 256 or
                  BlockType::block_size == 512);
    static constexpr uint64_t block_size = BlockType::block_size;
    static constexpr uint64_t words_in_block = block_size / 64;

    mutable_bitmap() {}

    void build(uint64_t const* in, uint64_t num_64bit_words) {
        assert(num_64bit_words * 64 <= (1ULL << 24) * block_size);
        uint64_t num_blocks =
            std::ceil(static_cast<double>(num_64bit_words) / words_in_block);
        std::vector<uint16_t> prefix_sums_input;
        prefix_sums_input.reserve(num_blocks);
        m_bits.resize(words_in_block * num_blocks,
                      0);  // pad to a multiple of words_in_block
        std::memcpy(m_bits.data(), in, num_64bit_words * sizeof(uint64_t));
        for (uint64_t i = 0; i != m_bits.size(); i += words_in_block) {
            uint16_t val = 0;
            for (uint64_t j = 0; j != words_in_block; ++j) {
                val += __builtin_popcountll(m_bits[i + j]);
            }
            assert(val <= block_size);
            prefix_sums_input.push_back(val);
        }
        m_index.build(prefix_sums_input.data(), prefix_sums_input.size());
    }

    static std::string name() {
        return "mutable_bitmap_" + std::to_string(block_size) + "<" +
               SearchablePrefixSums::name() + "," +
               print_rank_mode(BlockType::rank_mode) + "," +
               print_select_mode(BlockType::select_mode) + ">";
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

    inline bool access(uint64_t i) const {
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
        bool bit = access(i);
        uint64_t word = i / 64;
        uint64_t offset = i & 63;
        uint64_t block = i / block_size;
        m_bits[word] ^= 1ULL << offset;
        m_index.update(block, bit);
    }

    uint64_t rank(uint64_t i) const {
        assert(i < size());
        uint64_t block = i / block_size;
        uint64_t offset = i & (block_size - 1);
        return (block ? m_index.sum(block - 1) : 0) +
               BlockType::rank(&m_bits[words_in_block * block], offset);
    }

    uint64_t select(uint64_t i) const {
        assert(i < size());
        auto [block, sum] = m_index.search(i);
        uint64_t offset = i - sum;  // i - m_index.sum(block - 1);
        return block_size * block +
               BlockType::select(&m_bits[words_in_block * block], offset);
    }

private:
    SearchablePrefixSums m_index;
    std::vector<uint64_t> m_bits;
};

}  // namespace dyrs
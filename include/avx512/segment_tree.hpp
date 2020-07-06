#pragma once

#include <cassert>
#include <cmath>
#include <vector>

#include "segment_tree_macros.hpp"

namespace dyrs::avx512 {

template <uint32_t Height = 1>
struct segment_tree {
    static_assert(Height > 0 and Height <= 3);

    static constexpr uint64_t height(const uint64_t n) {
        if (n <= (1ULL << 9)) return 1;
        if (n <= (1ULL << 17)) return 2;
        if (n <= (1ULL << 24)) return 3;
        return 0;
    }

    segment_tree()
        : m_size(0), m_num_nodes_per_level(nullptr), m_ptr(nullptr) {}

    /* input is an intger array where each value is in [0,2^8] */
    void build(uint16_t const* input, uint64_t n) {
        assert(n > 0 and n <= (1ULL << 24));
        m_size = n;
        std::vector<uint32_t> num_nodes_per_level(Height);
        uint64_t m = n;
        uint64_t total_bytes = 0;
        int h = Height;

        /* Height is always at most 3 for bitvectors up to 2^32 bits*/
        {
            m = std::ceil(static_cast<double>(m) / node512::fanout);
            num_nodes_per_level[--h] = m;
            total_bytes += m * node512::bytes;
            if (m > 1) {
                m = std::ceil(static_cast<double>(m) / node256::fanout);
                num_nodes_per_level[--h] = m;
                total_bytes += m * node256::bytes;
            }
            if (m > 1) {
                m = std::ceil(static_cast<double>(m) / node128::fanout);
                num_nodes_per_level[--h] = m;
                total_bytes += m * node128::bytes;
            }
            assert(m == 1);
        }

        total_bytes += Height * sizeof(uint32_t);
        m_data.resize(total_bytes);

        m_num_nodes_per_level = reinterpret_cast<uint32_t*>(m_data.data());
        for (uint32_t i = 0; i != Height; ++i) {
            m_num_nodes_per_level[i] = num_nodes_per_level[i];
        }
        m_ptr = m_data.data() + Height * sizeof(uint32_t);

        uint8_t* begin = m_data.data() + total_bytes;  // end
        uint64_t step = 1;
        h = Height;

        {
            std::vector<node512::key_type> tmp(node512::fanout);
            uint64_t nodes = num_nodes_per_level[--h];
            begin -= nodes * node512::bytes;
            uint8_t* out = begin;
            for (uint64_t i = 0, base = 0; i != nodes;
                 ++i, base += node512::fanout) {
                for (uint64_t j = 0; j != node512::fanout and base + j < n;
                     ++j) {
                    uint16_t val = input[base + j];
                    assert(val <= 256);
                    tmp[j] = val;
                }
                node512::build(tmp.data(), out);
                out += node512::bytes;
            }
            step *= node512::fanout;
        }

        if (h > 0) {
            std::vector<node256::key_type> tmp(node256::fanout);
            uint64_t nodes = num_nodes_per_level[--h];
            begin -= nodes * node256::bytes;
            uint8_t* out = begin;
            for (uint64_t i = 0, base = 0; i != nodes; ++i) {
                for (node256::key_type& x : tmp) {
                    node256::key_type sum = 0;
                    for (uint64_t k = 0; k != step and base + k < n; ++k) {
                        sum += input[base + k];
                    }
                    x = sum;
                    base += step;
                }

                // shift right by 1
                for (int i = node256::fanout - 1; i > 0; --i) {
                    tmp[i] = tmp[i - 1];
                }
                tmp[0] = 0;

                node256::build(tmp.data(), out);
                out += node256::bytes;
            }
            step *= node256::fanout;
        }

        if (h > 0) {
            std::vector<node128::key_type> tmp(node128::fanout);
            uint64_t nodes = num_nodes_per_level[--h];
            begin -= nodes * node128::bytes;
            uint8_t* out = begin;
            for (uint64_t i = 0, base = 0; i != nodes; ++i) {
                for (node128::key_type& x : tmp) {
                    node128::key_type sum = 0;
                    for (uint64_t k = 0; k != step and base + k < n; ++k) {
                        sum += input[base + k];
                    }
                    x = sum;
                    base += step;
                }

                // shift right by 1
                for (int i = node128::fanout - 1; i > 0; --i) {
                    tmp[i] = tmp[i - 1];
                }
                tmp[0] = 0;

                node128::build(tmp.data(), out);
                out += node128::bytes;
            }
        }

        assert(h == 0);
    }

    static std::string name() {
        return "avx512::segment_tree";
    }

    uint64_t bytes() const {
        return sizeof(m_size) + sizeof(m_num_nodes_per_level) + sizeof(m_ptr) +
               essentials::vec_bytes(m_data);
    }

    uint32_t size() const {
        return m_size;
    }

    uint64_t sum(uint64_t i) const {
        assert(i < size());
        if constexpr (Height == 1) { DYRS_AVX512_SUM_H1 }
        if constexpr (Height == 2) { DYRS_AVX512_SUM_H2 }
        if constexpr (Height == 3) { DYRS_AVX512_SUM_H3 }
    }

    void update(uint64_t i, int8_t delta) {
        assert(i < size());
        if constexpr (Height == 1) { DYRS_AVX512_UPDATE_H1 }
        if constexpr (Height == 2) { DYRS_AVX512_UPDATE_H2 }
        if constexpr (Height == 3) { DYRS_AVX512_UPDATE_H3 }
    }

    uint64_t search(uint64_t x) const {
        if constexpr (Height == 1) { DYRS_AVX512_SEARCH_H1 }
        if constexpr (Height == 2) { DYRS_AVX512_SEARCH_H2 }
        if constexpr (Height == 3) { DYRS_AVX512_SEARCH_H3 }
    }

private:
    uint32_t m_size;
    uint32_t* m_num_nodes_per_level;
    uint8_t* m_ptr;
    std::vector<uint8_t> m_data;
};

}  // namespace dyrs::avx512
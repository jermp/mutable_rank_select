#pragma once

#include <cassert>
#include <cmath>
#include <vector>

#include "util.hpp"
#include "segment_tree_macros.hpp"

namespace mrs::avx2 {

template <uint32_t Height = 1>
struct segment_tree {
    static_assert(Height > 0 and Height <= 4);

    static constexpr uint64_t height(const uint64_t n) {
        if (n <= (1ULL << 7)) return 1;
        if (n <= (1ULL << 13)) return 2;
        if (n <= (1ULL << 19)) return 3;
        if (n <= (1ULL << 24)) return 4;
        return 0;
    }

    segment_tree()
        : m_size(0), m_num_nodes_per_level(nullptr), m_ptr(nullptr) {}

    void build(uint16_t const* input, uint64_t n) {
        assert(n > 0 and n <= (1ULL << 24));
        m_size = n;
        std::vector<uint32_t> num_nodes_per_level(Height);
        uint64_t m = n;
        uint64_t total_bytes = 0;
        int h = Height;

        {
            m = std::ceil(static_cast<double>(m) / node128::fanout);
            num_nodes_per_level[--h] = m;
            total_bytes += m * node128::bytes;

            for (int twice = 0; twice != 2; ++twice) {
                if (m > 1) {
                    m = std::ceil(static_cast<double>(m) / node64::fanout);
                    num_nodes_per_level[--h] = m;
                    total_bytes += m * node64::bytes;
                }
            }

            if (m > 1) {
                m = std::ceil(static_cast<double>(m) / node32::fanout);
                num_nodes_per_level[--h] = m;
                total_bytes += m * node32::bytes;
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
            std::vector<node128::key_type> tmp(node128::fanout);
            uint64_t nodes = num_nodes_per_level[--h];
            begin -= nodes * node128::bytes;
            uint8_t* out = begin;
            for (uint64_t i = 0, base = 0; i != nodes;
                 ++i, base += node128::fanout) {
                for (uint64_t j = 0; j != node128::fanout and base + j < n;
                     ++j) {
                    uint16_t val = input[base + j];
                    assert(val <= 512);
                    tmp[j] = val;
                }
                node128::build(tmp.data(), out);
                out += node128::bytes;
            }
            step *= node128::fanout;
        }

        for (int twice = 0; twice != 2; ++twice) {
            if (h > 0) {
                std::vector<node64::key_type> tmp(node64::fanout);
                uint64_t nodes = num_nodes_per_level[--h];
                begin -= nodes * node64::bytes;
                uint8_t* out = begin;
                for (uint64_t i = 0, base = 0; i != nodes; ++i) {
                    for (node64::key_type& x : tmp) {
                        node64::key_type sum = 0;
                        for (uint64_t k = 0; k != step and base + k < n; ++k) {
                            sum += input[base + k];
                        }
                        x = sum;
                        base += step;
                    }

                    // shift right by 1
                    for (int i = node64::fanout - 1; i > 0; --i) {
                        tmp[i] = tmp[i - 1];
                    }
                    tmp[0] = 0;

                    node64::build(tmp.data(), out);
                    out += node64::bytes;
                }
                step *= node64::fanout;
            }
        }

        if (h > 0) {
            std::vector<node32::key_type> tmp(node32::fanout);
            uint64_t nodes = num_nodes_per_level[--h];
            begin -= nodes * node32::bytes;
            uint8_t* out = begin;
            for (uint64_t i = 0, base = 0; i != nodes; ++i) {
                for (node32::key_type& x : tmp) {
                    node32::key_type sum = 0;
                    for (uint64_t k = 0; k != step and base + k < n; ++k) {
                        sum += input[base + k];
                    }
                    x = sum;
                    base += step;
                }

                // shift right by 1
                for (int i = node32::fanout - 1; i > 0; --i) {
                    tmp[i] = tmp[i - 1];
                }
                tmp[0] = 0;

                node32::build(tmp.data(), out);
                out += node32::bytes;
            }
        }

        assert(h == 0);
    }

    static std::string name() {
        return "avx2::segment_tree";
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
        if constexpr (Height == 1) { DYRS_AVX2_SUM_H1 }
        if constexpr (Height == 2) { DYRS_AVX2_SUM_H2 }
        if constexpr (Height == 3) { DYRS_AVX2_SUM_H3 }
        if constexpr (Height == 4) { DYRS_AVX2_SUM_H4 }
    }

    void update(uint64_t i,
                bool sign  // the sign of the update: 0 for +1, or 1 for -1
    ) {
        assert(i < size());
        if constexpr (Height == 1) { DYRS_AVX2_UPDATE_H1 }
        if constexpr (Height == 2) { DYRS_AVX2_UPDATE_H2 }
        if constexpr (Height == 3) { DYRS_AVX2_UPDATE_H3 }
        if constexpr (Height == 4) { DYRS_AVX2_UPDATE_H4 }
    }

    search_result search(uint64_t x) const {
        if constexpr (Height == 1) { DYRS_AVX2_SEARCH_H1 }
        if constexpr (Height == 2) { DYRS_AVX2_SEARCH_H2 }
        if constexpr (Height == 3) { DYRS_AVX2_SEARCH_H3 }
        if constexpr (Height == 4) { DYRS_AVX2_SEARCH_H4 }
    }

private:
    uint32_t m_size;
    uint32_t* m_num_nodes_per_level;
    uint8_t* m_ptr;
    std::vector<uint8_t> m_data;
};

}  // namespace mrs::avx2
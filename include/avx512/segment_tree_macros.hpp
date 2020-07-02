#pragma once

#define DYRS_AVX512_SUM_H1      \
    node512 node1(m_ptr);       \
    uint64_t s1 = node1.sum(i); \
    return s1;

#define DYRS_AVX512_SUM_H2                                           \
    uint64_t child1 = i / node512::fanout;                           \
    uint64_t child2 = i % node512::fanout;                           \
    node256 node1(m_ptr);                                            \
    node512 node2(m_ptr + node256::bytes + child1 * node512::bytes); \
    uint64_t s1 = node1.sum(child1);                                 \
    uint64_t s2 = node2.sum(child2);                                 \
    return s1 + s2;

#define DYRS_AVX512_SUM_H3                                               \
    constexpr uint64_t B1 = node512::fanout * node256::fanout;           \
    uint64_t child1 = i / B1;                                            \
    uint64_t child2 = (i % B1) / node512::fanout;                        \
    uint64_t child3 = (i % B1) % node512::fanout;                        \
    node128 node1(m_ptr);                                                \
    node256 node2(m_ptr + node128::bytes + child1 * node256::bytes);     \
    node512 node3(m_ptr + node128::bytes +                               \
                  m_num_nodes_per_level[1] * node256::bytes +            \
                  (child2 + child1 * node256::fanout) * node512::bytes); \
    uint64_t s1 = node1.sum(child1);                                     \
    uint64_t s2 = node2.sum(child2);                                     \
    uint64_t s3 = node3.sum(child3);                                     \
    return s1 + s2 + s3;

#define DYRS_AVX512_UPDATE_H1 \
    node512 node1(m_ptr);     \
    node1.update(i, delta);

#define DYRS_AVX512_UPDATE_H2                                        \
    uint64_t child1 = i / node512::fanout;                           \
    uint64_t child2 = i % node512::fanout;                           \
    node256 node1(m_ptr);                                            \
    node512 node2(m_ptr + node256::bytes + child1 * node512::bytes); \
    node1.update(child1 + 1, delta);                                 \
    node2.update(child2, delta);

#define DYRS_AVX512_UPDATE_H3                                            \
    constexpr uint64_t B1 = node512::fanout * node256::fanout;           \
    uint64_t child1 = i / B1;                                            \
    uint64_t child2 = (i % B1) / node512::fanout;                        \
    uint64_t child3 = (i % B1) % node512::fanout;                        \
    node128 node1(m_ptr);                                                \
    node256 node2(m_ptr + node128::bytes + child1 * node256::bytes);     \
    node512 node3(m_ptr + node128::bytes +                               \
                  m_num_nodes_per_level[1] * node256::bytes +            \
                  (child2 + child1 * node256::fanout) * node512::bytes); \
    node1.update(child1 + 1, delta);                                     \
    node2.update(child2 + 1, delta);                                     \
    node3.update(child3, delta);

#define DYRS_AVX512_SEARCH_H1      \
    node512 node1(m_ptr);          \
    uint64_t i1 = node1.search(x); \
    return i1;

#define DYRS_AVX512_SEARCH_H2                                    \
    node256 node1(m_ptr);                                        \
    uint64_t i1 = node1.search(x) - 1;                           \
    x -= node1.sum(i1);                                          \
    node512 node2(m_ptr + node256::bytes + i1 * node512::bytes); \
    uint64_t i2 = node2.search(x);                               \
    return i1 * node512::fanout + i2;

#define DYRS_AVX512_SEARCH_H3                                    \
    node128 node1(m_ptr);                                        \
    uint64_t i1 = node1.search(x) - 1;                           \
    x -= node1.sum(i1);                                          \
    node256 node2(m_ptr + node128::bytes + i1 * node256::bytes); \
    uint64_t i2 = node2.search(x) - 1;                           \
    x -= node2.sum(i2);                                          \
    node512 node3(m_ptr + node128::bytes +                       \
                  m_num_nodes_per_level[1] * node256::bytes +    \
                  (i2 + i1 * node256::fanout) * node512::bytes); \
    uint64_t i3 = node3.search(x);                               \
    return i1 * node512::fanout * node256::fanout + i2 * node512::fanout + i3;

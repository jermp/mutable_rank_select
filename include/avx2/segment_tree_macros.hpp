#pragma once

#define DYRS_AVX2_SUM_H1        \
    node128 node1(m_ptr);       \
    uint64_t s1 = node1.sum(i); \
    return s1;

#define DYRS_AVX2_SUM_H2                                            \
    uint64_t child1 = i / node128::fanout;                          \
    uint64_t child2 = i % node128::fanout;                          \
    node64 node1(m_ptr);                                            \
    node128 node2(m_ptr + node64::bytes + child1 * node128::bytes); \
    uint64_t s1 = node1.sum(child1);                                \
    uint64_t s2 = node2.sum(child2);                                \
    return s1 + s2;

#define DYRS_AVX2_SUM_H3                                                   \
    constexpr uint64_t B1 = node128::fanout * node64::fanout;              \
    uint64_t child1 = i / B1;                                              \
    uint64_t child2 = (i % B1) / node128::fanout;                          \
    uint64_t child3 = (i % B1) % node128::fanout;                          \
    node64 node1(m_ptr);                                                   \
    node64 node2(m_ptr + (child1 + 1) * node64::bytes);                    \
    node128 node3(m_ptr + (1 + m_num_nodes_per_level[1]) * node64::bytes + \
                  (child2 + child1 * node64::fanout) * node128::bytes);    \
    uint64_t s1 = node1.sum(child1);                                       \
    uint64_t s2 = node2.sum(child2);                                       \
    uint64_t s3 = node3.sum(child3);                                       \
    return s1 + s2 + s3;

#define DYRS_AVX2_SUM_H4                                                  \
    constexpr uint64_t B1 = node128::fanout * node64::fanout;             \
    constexpr uint64_t B2 = B1 * node64::fanout;                          \
    uint64_t child1 = i / B2;                                             \
    uint64_t child2 = (i % B2) / B1;                                      \
    uint64_t child3 = (i % B1) / node128::fanout;                         \
    uint64_t child4 = (i % B1) % node128::fanout;                         \
    node32 node1(m_ptr);                                                  \
    node64 node2(m_ptr + node32::bytes + child1 * node64::bytes);         \
    node64 node3(m_ptr + node32::bytes +                                  \
                 m_num_nodes_per_level[1] * node64::bytes +               \
                 (child2 + child1 * node64::fanout) * node64::bytes);     \
    node128 node4(m_ptr + node32::bytes +                                 \
                  (m_num_nodes_per_level[1] + m_num_nodes_per_level[2]) * \
                      node64::bytes +                                     \
                  (child3 + child2 * node64::fanout +                     \
                   child1 * node64::fanout * node64::fanout) *            \
                      node128::bytes);                                    \
    uint64_t s1 = node1.sum(child1);                                      \
    uint64_t s2 = node2.sum(child2);                                      \
    uint64_t s3 = node3.sum(child3);                                      \
    uint64_t s4 = node4.sum(child4);                                      \
    return s1 + s2 + s3 + s4;

#define DYRS_AVX2_UPDATE_H1 \
    node128 node1(m_ptr);   \
    node1.update(i, delta);

#define DYRS_AVX2_UPDATE_H2                                         \
    uint64_t child1 = i / node128::fanout;                          \
    uint64_t child2 = i % node128::fanout;                          \
    node64 node1(m_ptr);                                            \
    node128 node2(m_ptr + node64::bytes + child1 * node128::bytes); \
    node1.update(child1 + 1, delta);                                \
    node2.update(child2, delta);

#define DYRS_AVX2_UPDATE_H3                                                \
    constexpr uint64_t B1 = node128::fanout * node64::fanout;              \
    uint64_t child1 = i / B1;                                              \
    uint64_t child2 = (i % B1) / node128::fanout;                          \
    uint64_t child3 = (i % B1) % node128::fanout;                          \
    node64 node1(m_ptr);                                                   \
    node64 node2(m_ptr + (child1 + 1) * node64::bytes);                    \
    node128 node3(m_ptr + (1 + m_num_nodes_per_level[1]) * node64::bytes + \
                  (child2 + child1 * node64::fanout) * node128::bytes);    \
    node1.update(child1 + 1, delta);                                       \
    node2.update(child2 + 1, delta);                                       \
    node3.update(child3, delta);

#define DYRS_AVX2_UPDATE_H4                                               \
    constexpr uint64_t B1 = node128::fanout * node64::fanout;             \
    constexpr uint64_t B2 = B1 * node64::fanout;                          \
    uint64_t child1 = i / B2;                                             \
    uint64_t child2 = (i % B2) / B1;                                      \
    uint64_t child3 = (i % B1) / node128::fanout;                         \
    uint64_t child4 = (i % B1) % node128::fanout;                         \
    node32 node1(m_ptr);                                                  \
    node64 node2(m_ptr + node32::bytes + child1 * node64::bytes);         \
    node64 node3(m_ptr + node32::bytes +                                  \
                 m_num_nodes_per_level[1] * node64::bytes +               \
                 (child2 + child1 * node64::fanout) * node64::bytes);     \
    node128 node4(m_ptr + node32::bytes +                                 \
                  (m_num_nodes_per_level[1] + m_num_nodes_per_level[2]) * \
                      node64::bytes +                                     \
                  (child3 + child2 * node64::fanout +                     \
                   child1 * node64::fanout * node64::fanout) *            \
                      node128::bytes);                                    \
    node1.update(child1 + 1, delta);                                      \
    node2.update(child2 + 1, delta);                                      \
    node3.update(child3 + 1, delta);                                      \
    node4.update(child4, delta);

#define DYRS_AVX2_SEARCH_H1        \
    node128 node1(m_ptr);          \
    uint64_t i1 = node1.search(x); \
    return i1;

#define DYRS_AVX2_SEARCH_H2                                     \
    node64 node1(m_ptr);                                        \
    uint64_t i1 = node1.search(x) - 1;                          \
    x -= node1.sum(i1);                                         \
    node128 node2(m_ptr + node64::bytes + i1 * node128::bytes); \
    uint64_t i2 = node2.search(x);                              \
    return i1 * node128::fanout + i2;

#define DYRS_AVX2_SEARCH_H3                                                \
    node64 node1(m_ptr);                                                   \
    uint64_t i1 = node1.search(x) - 1;                                     \
    x -= node1.sum(i1);                                                    \
    node64 node2(m_ptr + (i1 + 1) * node64::bytes);                        \
    uint64_t i2 = node2.search(x) - 1;                                     \
    x -= node2.sum(i2);                                                    \
    node128 node3(m_ptr + (1 + m_num_nodes_per_level[1]) * node64::bytes + \
                  (i2 + i1 * node64::fanout) * node128::bytes);            \
    uint64_t i3 = node3.search(x);                                         \
    return i1 * node128::fanout * node64::fanout + i2 * node128::fanout + i3;

#define DYRS_AVX2_SEARCH_H4                                                 \
    node32 node1(m_ptr);                                                    \
    uint64_t i1 = node1.search(x) - 1;                                      \
    x -= node1.sum(i1);                                                     \
    node64 node2(m_ptr + node32::bytes + i1 * node64::bytes);               \
    uint64_t i2 = node2.search(x) - 1;                                      \
    x -= node2.sum(i2);                                                     \
    node64 node3(m_ptr + node32::bytes +                                    \
                 m_num_nodes_per_level[1] * node64::bytes +                 \
                 (i2 + i1 * node64::fanout) * node64::bytes);               \
    uint64_t i3 = node3.search(x) - 1;                                      \
    x -= node3.sum(i3);                                                     \
    node128 node4(                                                          \
        m_ptr + node32::bytes +                                             \
        (m_num_nodes_per_level[1] + m_num_nodes_per_level[2]) *             \
            node64::bytes +                                                 \
        (i3 + i2 * node64::fanout + i1 * node64::fanout * node64::fanout) * \
            node128::bytes);                                                \
    uint64_t i4 = node4.search(x);                                          \
    return i1 * node128::fanout * node64::fanout * node64::fanout +         \
           i2 * node128::fanout * node64::fanout + i3 * node128::fanout + i4;

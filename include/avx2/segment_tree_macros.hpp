#pragma once

namespace dyrs::avx2 {

#define SQARED(x) ((x) * (x))
#define CUBED(x) ((x) * (x) * (x))
#define BIQUADRATE(x) ((x) * (x) * (x) * (x))

#define SUM_H1                  \
    node128 node1(m_ptr);       \
    uint64_t s1 = node1.sum(i); \
    return s1;

#define SUM_H2                                                      \
    uint64_t child1 = i / node128::fanout;                          \
    uint64_t child2 = i % node128::fanout;                          \
    node64 node1(m_ptr);                                            \
    node128 node2(m_ptr + node64::bytes + child1 * node128::bytes); \
    uint64_t s1 = node1.sum(child1);                                \
    uint64_t s2 = node2.sum(child2);                                \
    return s1 + s2;

#define UPDATE_H1         \
    node128 node1(m_ptr); \
    node1.update(i, delta);

#define UPDATE_H2                                                   \
    uint64_t child1 = i / node128::fanout;                          \
    uint64_t child2 = i % node128::fanout;                          \
    node64 node1(m_ptr);                                            \
    node128 node2(m_ptr + node64::bytes + child1 * node128::bytes); \
    node1.update(child1 + 1, delta);                                \
    node2.update(child2, delta);

}  // namespace dyrs::avx2
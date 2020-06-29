#pragma once

namespace dyrs {

template <typename SummaryType, typename KeyType>
void build_node_prefix_sums(KeyType const* input, uint8_t* out,
                            uint64_t segment_size, uint64_t num_segments,
                            uint64_t bytes) {
    std::fill(out, out + bytes, 0);
    SummaryType* summary = reinterpret_cast<SummaryType*>(out);
    uint64_t summary_bytes = num_segments * sizeof(SummaryType);
    KeyType* keys = reinterpret_cast<KeyType*>(out + summary_bytes);
    summary[0] = 0;
    for (uint64_t i = 0; i != num_segments; ++i) {
        keys[0] = input[0];
        for (uint64_t j = 1; j != segment_size; ++j) {
            keys[j] = keys[j - 1] + input[j];
        }
        if (i + 1 < num_segments) {
            summary[i + 1] = summary[i] + keys[segment_size - 1];
        }
        input += segment_size;
        keys += segment_size;
    }
}

}  // namespace dyrs
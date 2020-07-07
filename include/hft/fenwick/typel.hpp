#ifndef __FENWICK_TYPEL_HPP__
#define __FENWICK_TYPEL_HPP__

#include "fenwick_tree.hpp"

namespace hft::fenwick {

/**
 * class TypeL - closer type compression and level-ordered node layout.
 * @sequence: sequence of integers.
 * @size: number of elements.
 * @BOUND: maximum value that @sequence can store.
 *
 */
template <size_t BOUND> class TypeL : public FenwickTree {
public:
  static constexpr size_t BOUNDSIZE = ceil_log2_plus1(BOUND);
  static_assert(BOUNDSIZE >= 1 && BOUNDSIZE <= 64, "Leaves can't be stored in a 64-bit word");

protected:
  const size_t Size, Levels;
  unique_ptr<size_t[]> Level;

  DArray<uint8_t> Tree8;
  DArray<uint16_t> Tree16;
  DArray<uint64_t> Tree64;

public:
  TypeL(uint64_t sequence[], size_t size)
      : Size(size), Levels(size != 0 ? lambda(size) + 2 : 1), Level(make_unique<size_t[]>(Levels)) {
    size_t typeEnd[3] = {0};
    Level[0] = 0;

    size_t j = (BOUNDSIZE <= 8) ? 0 : (BOUNDSIZE <= 16) ? 1 : 2;
    for (size_t i = 1; i < Levels; i++) {
      typeEnd[j] = Level[i] = (size + (1ULL << (i - 1))) / (1ULL << i) + Level[i - 1];

      if (i - 1 == 8 - BOUNDSIZE || i - 1 == 16 - BOUNDSIZE) {
        Level[i] = 0;
        j++;
      }
    }

    switch (Levels + BOUNDSIZE - 1) {
    case 17 ... 64:
      Tree64 = DArray<uint64_t>(typeEnd[2]);
    case 9 ... 16:
      Tree16 = DArray<uint16_t>(typeEnd[1]);
    default:
      Tree8 = DArray<uint8_t>(typeEnd[0]);
    }

    if (BOUNDSIZE <= 8) {
      fillTree<uint8_t, BOUNDSIZE, 8>(Tree8.get(), sequence);
      fillTree<uint16_t, 9, 16>(Tree16.get(), sequence);
      fillTree<uint64_t, 17, 32>(Tree64.get(), sequence);
      fillTree<uint64_t, 33, 64>(Tree64.get(), sequence);
    } else if (BOUNDSIZE <= 16) {
      fillTree<uint16_t, BOUNDSIZE, 16>(Tree16.get(), sequence);
      fillTree<uint64_t, 17, 32>(Tree64.get(), sequence);
      fillTree<uint64_t, 33, 64>(Tree64.get(), sequence);
    } else if (BOUNDSIZE <= 32) {
      fillTree<uint64_t, BOUNDSIZE, 32>(Tree64.get(), sequence);
      fillTree<uint64_t, 33, 64>(Tree64.get(), sequence);
    } else {
      fillTree<uint64_t, BOUNDSIZE, 64>(Tree64.get(), sequence);
    }
  }

  virtual uint64_t prefix(size_t idx) const {
    uint64_t sum = 0;

    while (idx != 0) {
      const int height = rho(idx);
      const size_t level_idx = idx >> (1 + height);
      const size_t tree_idx = Level[height] + level_idx;

      switch (height + BOUNDSIZE) {
      case 17 ... 64:
        sum += Tree64[tree_idx];
        break;
      case 9 ... 16:
        sum += Tree16[tree_idx];
        break;
      default:
        sum += Tree8[tree_idx];
      }

      idx = clear_rho(idx);
    }

    return sum;
  }

  virtual void add(size_t idx, int64_t inc) {
    while (idx <= Size) {
      const int height = rho(idx);
      const size_t level_idx = idx >> (1 + height);
      const size_t tree_idx = Level[height] + level_idx;

      switch (height + BOUNDSIZE) {
      case 17 ... 64:
        Tree64[tree_idx] += inc;
        break;
      case 9 ... 16:
        Tree16[tree_idx] += inc;
        break;
      default:
        Tree8[tree_idx] += inc;
      }

      idx += mask_rho(idx);
    }
  }

  using FenwickTree::find;
  virtual size_t find(uint64_t *val) const {
    size_t node = 0, idx = 0;

    for (size_t height = Levels - 2; height != SIZE_MAX; height--) {
      const size_t tree_idx = Level[height] + idx;

      idx <<= 1;

      uint64_t value = 0;
      switch (height + BOUNDSIZE) {
      case 17 ... 64:
        if (tree_idx >= Tree64.size())
          continue;
        value += Tree64[tree_idx];
        break;
      case 9 ... 16:
        if (tree_idx >= Tree16.size())
          continue;
        value += Tree16[tree_idx];
        break;
      default:
        if (tree_idx >= Tree8.size())
          continue;
        value += Tree8[tree_idx];
      }

      if (*val >= value) {
        idx++;
        *val -= value;
        node += 1ULL << height;
      }
    }

    return min(node, Size);
  }

  using FenwickTree::compFind;
  virtual size_t compFind(uint64_t *val) const {
    size_t node = 0, idx = 0;

    for (size_t height = Levels - 2; height != SIZE_MAX; height--) {
      const size_t tree_idx = Level[height] + idx;

      idx <<= 1;

      uint64_t value = BOUND << height;
      switch (height + BOUNDSIZE) {
      case 17 ... 64:
        if (tree_idx >= Tree64.size())
          continue;
        value -= Tree64[tree_idx];
        break;
      case 9 ... 16:
        if (tree_idx >= Tree16.size())
          continue;
        value -= Tree16[tree_idx];
        break;
      default:
        if (tree_idx >= Tree8.size())
          continue;
        value -= Tree8[tree_idx];
      }

      if (*val >= value) {
        idx++;
        *val -= value;
        node += 1ULL << height;
      }
    }

    return min(node, Size);
  }

  virtual size_t size() const { return Size; }

  virtual size_t bitCount() const {
    return sizeof(TypeL<BOUNDSIZE>) * 8 + Tree8.bitCount() - sizeof(Tree8) + Tree16.bitCount() -
           sizeof(Tree16) + Tree64.bitCount() - sizeof(Tree64) + Levels * sizeof(size_t) * 8;
  }

private:
  template <typename T, size_t s, size_t e> inline void fillTree(T *tree, uint64_t sequence[]) {
    for (size_t l = s - BOUNDSIZE; l < Levels - 1 && l <= e - BOUNDSIZE; l++) {
      for (size_t node = 1ULL << l; node <= Size; node += 1ULL << (l + 1)) {
        size_t sequence_idx = node - 1;
        T value = sequence[sequence_idx];

        size_t j = 0;
        switch (BOUNDSIZE) {
        case 1 ... 8:
          for (; j < l && j <= 8 - BOUNDSIZE; j++) {
            sequence_idx >>= 1;
            value += Tree8[Level[j] + sequence_idx];
          }
        case 9 ... 16:
          for (; j < l && j <= 16 - BOUNDSIZE; j++) {
            sequence_idx >>= 1;
            value += Tree16[Level[j] + sequence_idx];
          }
        case 17 ... 32:
          for (; j < l && j <= 32 - BOUNDSIZE; j++) {
            sequence_idx >>= 1;
            value += Tree64[Level[j] + sequence_idx];
          }
        default:
          for (; j < l && j <= 64 - BOUNDSIZE; j++) {
            sequence_idx >>= 1;
            value += Tree64[Level[j] + sequence_idx];
          }
        }

        tree[Level[l] + (node >> (l + 1))] = value;
      }
    }
  }

private:
  friend std::ostream &operator<<(std::ostream &os, const TypeL<BOUND> &ft) {
    const uint64_t nsize = hton((uint64_t)ft.Size);
    os.write((char *)&nsize, sizeof(uint64_t));

    const uint64_t nlevels = hton((uint64_t)ft.Levels);
    os.write((char *)&nlevels, sizeof(uint64_t));

    for (size_t i = 0; i < ft.Levels; ++i) {
      const uint64_t nlevel = hton((uint64_t)ft.Level[i]);
      os.write((char *)&nlevel, sizeof(uint64_t));
    }

    return os << ft.Tree8 << ft.Tree16 << ft.Tree64;
  }

  friend std::istream &operator>>(std::istream &is, TypeL<BOUND> &ft) {
    uint64_t nsize;
    is.read((char *)(&nsize), sizeof(uint64_t));
    ft.Size = ntoh(nsize);

    uint64_t nlevels;
    is.read((char *)&nlevels, sizeof(uint64_t));
    ft.Levels = ntoh(nlevels);

    ft.Level = make_unique<size_t[]>(ft.Levels);
    for (size_t i = 0; i < ft.Levels; ++i) {
      uint64_t nlevel;
      is.read((char *)&nlevel, sizeof(uint64_t));
      ft.Level[i] = ntoh(nlevel);
    }

    return is >> ft.Tree8 >> ft.Tree16 >> ft.Tree64;
  }
};

} // namespace hft::fenwick

#endif // __FENWICK_TYPEL_HPP__

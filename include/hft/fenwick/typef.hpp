#ifndef __FENWICK_TYPEF_HPP__
#define __FENWICK_TYPEF_HPP__

#include "fenwick_tree.hpp"

namespace hft::fenwick {

/**
 * class TypeF - closer type compression and classical node layout.
 * @sequence: sequence of integers.
 * @size: number of elements.
 * @BOUND: maximum value that @sequence can store.
 *
 */
template <size_t BOUND> class TypeF : public FenwickTree {
public:
  static constexpr size_t BOUNDSIZE = ceil_log2_plus1(BOUND);
  static_assert(BOUNDSIZE >= 1 && BOUNDSIZE <= 64, "Leaves can't be stored in a 64-bit word");

protected:
  const size_t Size;
  DArray<uint8_t> Tree;

public:
  TypeF(uint64_t sequence[], size_t size) : Size(size), Tree(pos(size + 1)) {
    for (size_t i = 1; i <= Size; i++) {
      const size_t bytepos = pos(i);

      switch (BOUNDSIZE + rho(i)) {
      case 17 ... 64:
        reinterpret_cast<auint64_t &>(Tree[bytepos]) = sequence[i - 1];
        break;
      case 9 ... 16:
        reinterpret_cast<auint16_t &>(Tree[bytepos]) = sequence[i - 1];
        break;
      default:
        reinterpret_cast<auint8_t &>(Tree[bytepos]) = sequence[i - 1];
      }
    }

    for (size_t m = 2; m <= Size; m <<= 1) {
      for (size_t idx = m; idx <= Size; idx += m) {
        const size_t left = pos(idx);
        const size_t right = pos(idx - m / 2);

        switch (BOUNDSIZE + rho(idx)) {
        case 17 ... 64:
          switch (BOUNDSIZE + rho(idx - m / 2)) {
          case 17 ... 64:
            reinterpret_cast<auint64_t &>(Tree[left]) +=
                *reinterpret_cast<auint64_t *>(&Tree[right]);
            break;
          case 9 ... 16:
            reinterpret_cast<auint64_t &>(Tree[left]) +=
                *reinterpret_cast<auint16_t *>(&Tree[right]);
            break;
          default:
            reinterpret_cast<auint64_t &>(Tree[left]) +=
                *reinterpret_cast<auint8_t *>(&Tree[right]);
          }
          break;
        case 9 ... 16:
          switch (BOUNDSIZE + rho(idx - m / 2)) {
          case 9 ... 16:
            reinterpret_cast<auint16_t &>(Tree[left]) +=
                *reinterpret_cast<auint16_t *>(&Tree[right]);
            break;
          default:
            reinterpret_cast<auint16_t &>(Tree[left]) +=
                *reinterpret_cast<auint8_t *>(&Tree[right]);
          }
          break;
        default:
          reinterpret_cast<auint8_t &>(Tree[left]) +=
            *reinterpret_cast<auint8_t *>(&Tree[right]);
        }
      }
    }
  }

  virtual uint64_t prefix(size_t idx) const {
    uint64_t sum = 0;

    while (idx != 0) {
      const size_t bytepos = pos(idx);

      switch (BOUNDSIZE + rho(idx)) {
      case 17 ... 64:
        sum += *reinterpret_cast<auint64_t *>(&Tree[bytepos]);
        break;
      case 9 ... 16:
        sum += *reinterpret_cast<auint16_t *>(&Tree[bytepos]);
        break;
      default:
        sum += *reinterpret_cast<auint8_t *>(&Tree[bytepos]);
      }

      idx = clear_rho(idx);
    }

    return sum;
  }

  virtual void add(size_t idx, int64_t inc) {
    while (idx <= Size) {
      const size_t bytepos = pos(idx);

      switch (BOUNDSIZE + rho(idx)) {
      case 17 ... 64:
        *reinterpret_cast<auint64_t *>(&Tree[bytepos]) += inc;
        break;
      case 9 ... 16:
        *reinterpret_cast<auint16_t *>(&Tree[bytepos]) += inc;
        break;
      default:
        *reinterpret_cast<auint8_t *>(&Tree[bytepos]) += inc;
      }

      idx += mask_rho(idx);
    }
  }

  using FenwickTree::find;
  virtual size_t find(uint64_t *val) const {
    size_t node = 0;

    for (size_t m = mask_lambda(Size); m != 0; m >>= 1) {
      if (node + m > Size)
        continue;

      const size_t bytepos = pos(node + m);
      const int bitlen = BOUNDSIZE + rho(node + m);

      uint64_t value;
      switch (bitlen) {
      case 17 ... 64:
        value = *reinterpret_cast<auint64_t *>(&Tree[bytepos]);
        break;
      case 9 ... 16:
        value = *reinterpret_cast<auint16_t *>(&Tree[bytepos]);
        break;
      default:
        value = *reinterpret_cast<auint8_t *>(&Tree[bytepos]);
      }

      if (*val >= value) {
        node += m;
        *val -= value;
      }
    }

    return node;
  }

  using FenwickTree::compFind;
  virtual size_t compFind(uint64_t *val) const {
    size_t node = 0;

    for (size_t m = mask_lambda(Size); m != 0; m >>= 1) {
      if (node + m > Size)
        continue;

      const size_t bytepos = pos(node + m);
      const int height = rho(node + m);

      uint64_t value = BOUND << height;
      switch (BOUNDSIZE + height) {
      case 17 ... 64:
        value -= *reinterpret_cast<auint64_t *>(&Tree[bytepos]);
        break;
      case 9 ... 16:
        value -= *reinterpret_cast<auint16_t *>(&Tree[bytepos]);
        break;
      default:
        value -= *reinterpret_cast<auint8_t *>(&Tree[bytepos]);
      }

      if (*val >= value) {
        node += m;
        *val -= value;
      }
    }

    return node;
  }

  virtual size_t size() const { return Size; }

  virtual size_t bitCount() const {
    return sizeof(TypeF<BOUNDSIZE>) * 8 + Tree.bitCount() - sizeof(Tree);
  }

private:
  inline static size_t pos(size_t idx) {
    idx--;
    return idx + (idx >> (BOUNDSIZE <= 8 ? (8 - BOUNDSIZE + 1) : 0)) +
           (idx >> (BOUNDSIZE <= 16 ? (16 - BOUNDSIZE + 1) : 0)) * 6;
  }

  friend std::ostream &operator<<(std::ostream &os, const TypeF<BOUND> &ft) {
    const uint64_t nsize = hton((uint64_t)ft.Size);
    os.write((char *)&nsize, sizeof(uint64_t));

    return os << ft.Tree;
  }

  friend std::istream &operator>>(std::istream &is, TypeF<BOUND> &ft) {
    uint64_t nsize;
    is.read((char *)(&nsize), sizeof(uint64_t));

    ft.Size = ntoh(nsize);
    return is >> ft.Tree;
  }
};

} // namespace hft::fenwick

#endif // __FENWICK_TYPEF_HPP__

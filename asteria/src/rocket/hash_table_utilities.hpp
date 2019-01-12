// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_HASH_TABLE_UTILITIES_HPP_
#define ROCKET_HASH_TABLE_UTILITIES_HPP_

#include "assert.hpp"
#include "utilities.hpp"

namespace rocket {

template<typename bucketT>
  inline bucketT * get_probing_origin(bucketT *begin, bucketT *end, size_t hval) noexcept
  {
    ROCKET_ASSERT(begin < end);
    // Multiplication is faster than division.
    const auto seed = hval * 0x9E3779B9 / 2;
    const auto ratio = static_cast<double>(static_cast<long>(seed) & 0x7FFFFFFF) / 0x80000000;
    const auto off = static_cast<ptrdiff_t>(static_cast<double>(end - begin) * ratio);
    ROCKET_ASSERT((0 <= off) && (off < end - begin));
    return begin + off;
  }

template<typename bucketT, typename predT>
  inline bucketT * linear_probe(bucketT *begin, bucketT *to, bucketT *from, bucketT *end, const predT &pred) noexcept(noexcept(::std::declval<const predT>()(::std::declval<bucketT &>())))
  {
    ROCKET_ASSERT(begin <= to);
    ROCKET_ASSERT(to <= from);
    ROCKET_ASSERT(from <= end);
    // Phase 1: Probe from `from` to `end`.
    for(auto bkt = from; bkt != end; ++bkt) {
      if(!*bkt || pred(*bkt)) {
        return bkt;
      }
    }
    // Phase 2: Probe from `begin` to `to`.
    for(auto bkt = begin; bkt != to; ++bkt) {
      if(!*bkt || pred(*bkt)) {
        return bkt;
      }
    }
    // The table is full and no desired bucket has been found so far.
    return nullptr;
  }

}

#endif